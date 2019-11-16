/*
Copyright (c) 2019, Leo Tomura
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, 
  this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, 
  this list of conditions and the following disclaimer in the documentation 
  and/or other materials provided with the distribution.
* Neither the name of the Leo Tomura nor the names of its contributors 
  may be used to endorse or promote products derived from this software 
  without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL LEO TOMURA BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "denta-kun.h"

const Poly nullPoly = {
	.size = -1,
	.items = NULL;
};

int isNullPoly(unmut Poly v){
	return v.size == nullPoly.size;
}

Poly polyDup(unmut Poly poly){
	Poly retval = {
		.size = poly.size;
		.items = malloc(sizeof(Item)*poly.size)
	};
	int i;
	for(i = 0,i < retval.size;i++){
		retval.items[i] = _copyItem(poly.item[i]);
	}
	return retval;
}

void polyFree(mut Poly v){
	Item *now = v.items;
	while(now){
		free(now.indexes);
		free(now.degrees);
		Item *tmp = now;
		now = now->next;
		free(tmp);
	}
}

double poly2Double(unmut Poly poly){
	int i;
	double val = 0;
	for(i = 0;i < poly.size;i++){
		if(poly.items[i].size > 0){
			fprintf(stderr,"Trying to convert Poly to double but Poly has variable\n");
			DIE;
		}
		val += poly.items[i].coefficient;
	}
	return val;
}

Item _copyItem(unmut Item item){
	Item retval = {
		.size = item.size,
		.coefficient = item.coefficient,
		.degrees = malloc(sizeof(item.size))
	};
	memcpy(retval.degrees,item.degrees,item.size);
	return retval; 
}

Poly appendItem2Poly(unmut Poly poly,unmut Item item){
	Poly retval;
	retval.size = poly.size + 1;
	retval.items = malloc(sizeof(Item)*(retval.size));
	Item *ptr = retval.items;
	int i;
	for(i = 0;i < poly.size;i++){
		Item tmp = _copyItem(*items++);
		memcpy(ptr++,&tmp,sizeof(Item));
	}
	Item tmp = _copyItem(item);
	memcpy(ptr,tmp,sizeof(Item));
	return retval;
}

int _sortFunc_LEX(const Item *v1,const Item *v2){
	int degree_v1 = 0;
	int degree_v2 = 0;
	int i;
	for(i = 0;i < v1->size;i++){
		degree_v1 += v1->degrees[i];
	}
	for(i = 0;i < v2->size;i++){
		degree_v2 += v2->degrees[i];
	}
	if(degree_v1 != degree_v2){
		return (degree_v1 < degree_v2) ? -1 : +1;
	}
	for(i = 0;i < v1->size && i < v2->size;i++){
		if(v2->degrees[i] - v1->degrees[i] != 0){
			return (v2->degrees[i] - v1->degrees[i] > 0) ? -1 : +1;
		}
	}
	if(v1->size == v2->size){
		return 0;
	}else if(v2->size > v1->size){
		return -1;
	}else{
		return +1;
	} 
}
int _sortFunc_RLEX(const Item *v1,const Item *v2){
	int degree_v1 = 0;
	int degree_v2 = 0;
	int i;
	for(i = 0;i < v1->size;i++){
		degree_v1 += v1->degrees[i];
	}
	for(i = 0;i < v2->size;i++){
		degree_v2 += v2->degrees[i];
	}
	if(degree_v1 != degree_v2){
		return (degree_v1 < degree_v2) ? -1 : +1;
	}
	if(v2->size < v1->size){
		return -1;
	}else if(v2->size > v1->size){
		return +1;
	}
	/*v1->size == v2->size*/
	for(i = v1->size-1;i >= 0;i--){
		if(v2->degrees[i] - v1->degrees[i] != 0){
			return (v2->degrees[i] - v1->degrees[i] < 0) ? -1 : +1;
		}
	}
	return 0;
}
int _sortFunc_PLEX(const Item *v1,const Item *v2){
	for(i = 0;i < v1->size && i < v2->size;i++){
		if(v2->degrees[i] - v1->degrees[i] != 0){
			return (v2->degrees[i] - v1->degrees[i] > 0) ? -1 : +1;
		}
	}
	if(v1->size == v2->size){
		return 0;
	}else if(v2->size > v1->size){
		return -1;
	}else{
		return +1;
	} 
}

int _isSameMonomial(unmut Item v1,unmut Item v2){
	if(v1.size != v2.size){
		return 0;
	}
	int i;
	for(i=0;i < v1.size;i++){
		if(v1->degrees[i] != v2->degrees[i]){
			return 0;
		}
	}
	return 1;
}

Poly polySort(unmut Poly poly,MonomialOrder order){
	Poly toBeSorted = polyDup(poly);
	switch(order){
		LEX : {qsort(toBeSorted.item,sizeof(Item),_sortFunc_LEX); break;}
		RLEX : {qsort(toBeSorted.item,sizeof(Item),_sortFunc_RLEX); break;}
		PLEX : {qsort(toBeSorted.item,sizeof(Item),_sortFunc_PLEX); break;}
	}
	Poly retval = {
		.size = toBeSorted.size,
		.items = malloc(sizeof(Items) * toBeSorted.size)
	};
	int index = 0;
	int i,j;
	for(i = 0;i < toBeSorted.size;i++){
		retval.items[index] = toBeSorted.items[i];
		for(j = i + 1;j < toBeSorted.size && _isSameMonomial(toBeSorted.items[i],toBeSorted.items[j]);j++){
			retval.items[index].coefficient += toBeSorted.items[j].coefficient;
			free(toBeSorted.items[j].degrees);
		}
		i = j - 1;
		index++;
	}
	if(retval.size != index){
		retval.size = index;
		retval.items = realloc(retval.items,sizeof(Item)*retval.size);
	}
	return retval;
}

