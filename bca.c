/*
Copyright 2020, Leo Tomura

This file is part of Dentakun.

Dentakun is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Dentakun is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Dentakun.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "denta-kun.h"

#if BOOLEAN

#define EDGE_TYPE_PERIODIC (1)
#define EDGE_TYPE_REFLECTIVE (2)
#define EDGE_TYPE_FIXED (3)

#define BCA_TIME_INTERVAL (size)
//#define BCA_TIME_INTERVAL (13824)


char valueAt(char **buff,size_t size,int t,int i,int edge_type,char edge_value){
	if(i < 0 || i >= size){
		switch (edge_type){
			case EDGE_TYPE_PERIODIC : {
				if(i < 0){
					return buff[t][size + i];
				}else{
					return buff[t][i - size];
				}
			}break;
			case EDGE_TYPE_REFLECTIVE : {
				if(i < 0){
					return buff[t][0];
				}else{
					return buff[t][size-1];
				}
			}break;
			case EDGE_TYPE_FIXED : {
				return edge_value;
			}break;
		}
	}else{
		return buff[t][i];
	}
	DIE;
}

char nextValue(char **buff,size_t size,int t,int index,Poly poly,int edge_type,char edge_value){
	char retval = K_0;
	int i,j;
	for(i = 0;i < polySize(poly);i++){
		if(!cmpK(poly.ptr.items[i].coefficient,K_0)){
			continue;
		}
		for(j = 0;j < poly.ptr.items[i].size;j++){
			if(poly.ptr.items[i].degrees[j]){
				if(j == 0){
					if(!valueAt(buff,size,t-1,index,edge_type,edge_value)){
						goto zero;
					}
				}else if(j%2){
					/*odd i.e. left*/
					if(!valueAt(buff,size,t-1,index - (j/2) - 1,edge_type,edge_value)){
						goto zero;
					}
				}else{
					/*even i.e. right*/
					if(!valueAt(buff,size,t-1,index + (j/2),edge_type,edge_value)){
						goto zero;
					}
				}
			}
		}
		addK(retval,retval,K_1);
		continue;
		zero:
		addK(retval,retval,K_0);
	}
	return retval;
}

Poly _BCA(char **buff,size_t size,Poly poly,int edge_type,char edge_value){
	int t,i;
	//koko
	for(t = 1;t < BCA_TIME_INTERVAL;t++){
		for(i = 0;i < size ;i++){
			buff[t][i] = nextValue(buff,size,t,i,poly,edge_type,edge_value);
		}
	}
	Poly *retval = malloc(sizeof(Poly)*BCA_TIME_INTERVAL);
	for(i = 0;i < BCA_TIME_INTERVAL;i++){
		Poly *row = malloc(sizeof(Poly)*size);
		Poly onePoly = polyDup(zeroPoly);
		onePoly.ptr.items[0].coefficient = K_1;
		int j;
		for(j = 0;j < size;j++){
			if(buff[i][j]){
				row[j] = polyDup(onePoly);
			}else{
				row[j] = polyDup(zeroPoly);
			}
		}
		retval[i] = mkPolyArray(row,size);
	}
	return mkPolyArray(retval,BCA_TIME_INTERVAL);
}

Poly BCA(Poly arg,BlackBoard blackboard){
	Poly poly;
	int i;
	int edge_type = EDGE_TYPE_PERIODIC;
	char edge_value = 0;
	if(polyType(arg) != ARRAY){
		poly = arg;
	}else{
		Poly *array = unwrapPolyArray(arg);
		poly = polyDup(array[0]);
		for(i = 1;i < polySize(arg);i++){
			Poly tmp = polyAdd(poly,array[i]);
			polyFree(poly);
			poly = tmp;
		}
	}
	Poly _tmp = findFromBlackBoard(blackboard,"BCA_PERIODIC",strlen("BCA_PERIODIC"));
	if( !isNullPoly(_tmp) && !isZeroPoly(_tmp)){
		edge_type = EDGE_TYPE_PERIODIC;
		goto next;
	}
	_tmp = findFromBlackBoard(blackboard,"BCA_REFLECTIVE",strlen("BCA_REFLECTIVE"));
	if( !isNullPoly(_tmp) && !isZeroPoly(_tmp)){
		edge_type = EDGE_TYPE_REFLECTIVE;
		goto next;
	}
	_tmp = findFromBlackBoard(blackboard,"BCA_FIXED",strlen("BCA_FIXED"));
	if( !isNullPoly(_tmp) && !isZeroPoly(_tmp)){
		edge_type = EDGE_TYPE_FIXED;
		_tmp = findFromBlackBoard(blackboard,"BCA_FIXED_VALUE",strlen("BCA_FIXED_VALUE"));
		if(! isNullPoly(_tmp) && ! isZeroPoly(_tmp)){
			edge_value = 1;
		}
		goto next;
	}
	next : ;
	Poly _vector = findFromBlackBoard(blackboard,"BCA_INITIAL_STATE",strlen("BCA_INITIAL_STATE"));
	if(polyType(_vector) != ARRAY){
		fprintf(stderr,"BCA_INITIAL_STATE must be an array.");
	}
	size_t size = polySize(_vector);
	Poly *vector = unwrapPolyArray(_vector);
	char **buff = malloc(sizeof(char *)*BCA_TIME_INTERVAL);
	for(i = 0;i < BCA_TIME_INTERVAL;i++){
		buff[i] = calloc(sizeof(char) , size);
	}
	for(i = 0;i < size;i++){
		buff[0][i] = isZeroPoly(vector[i]) ? 0 : 1;
	}
	Poly retval = _BCA(buff,size,poly,edge_type,edge_value);
	return retval;
}

#endif
