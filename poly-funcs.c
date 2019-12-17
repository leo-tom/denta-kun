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
	.size = 0,
	.ptr.items = NULL
};

//[ y^5+y^4-11*y^3-17*y^2+9*y+17, -y^4+x*y+11*y^2-x+3*y-13, x^2-y^2+1 ]

extern int _isSameMonomial(unmut Item v1,unmut Item v2);
extern int _polycmp_LEX(const Item *v1,const Item *v2);
extern int _polycmp_RLEX(const Item *v1,const Item *v2);
extern int _polycmp_PLEX(const Item *v1,const Item *v2);
extern Item _copyItem(unmut Item item);
void polyFree(mut Poly v){
	return;
	size_t size = polySize(v);
	size_t i;
	if(polyType(v) == ARRAY){
		Poly *ptr = v.ptr.polies;
		for(i = 0;i < size;i++){
			polyFree(*ptr++);
		}
	}else{
		for(i=0;i < size;i++){
			free(v.ptr.items[i].degrees);
		}
	}
	if(size > 0){
		free(v.ptr.items);
	}
}
int isNullPoly(unmut Poly poly){
	return poly.size == nullPoly.size
			&& poly.ptr.items == nullPoly.ptr.items;
}
int isZeroPoly(unmut Poly poly){
	return polySize(poly) == 1 && !cmpK(poly.ptr.items[0].coefficient,K_0);
}
Poly polyDup(unmut Poly poly){
	Poly retval = {
		.size = poly.size
	};
	if(polyType(poly) == ARRAY){
		retval.ptr.items = malloc(sizeof(Poly)*polySize(poly));
		Poly *from = poly.ptr.polies;
		Poly *to = retval.ptr.polies;
		int i;
		for(i = 0;i < polySize(poly);i++){
			to[i] = polyDup(from[i]);
		}
	}else{
		retval.ptr.items = malloc(sizeof(Item)*polySize(poly));
		int i;
		for(i = 0;i < polySize(poly);i++){
			retval.ptr.items[i] = _copyItem(poly.ptr.items[i]);
		}
	}
	return retval;
}

double poly2Double(unmut Poly poly){
	if(poly.size != 1 || poly.ptr.items[0].size > 0){
		fprintf(stderr,"Trying to call poly2Double to : ");polyPrint(poly,stderr);fprintf(stderr,"\n");
		DIE;
	}
	return K2double(poly.ptr.items[0].coefficient);
}

void polyPrint(unmut Poly poly,FILE *fp){
	size_t size = polySize(poly);
	if(isNullPoly(poly)){
		fprintf(fp,"(null)");
		return;
	}else if(polyType(poly) == ARRAY){
		Poly *head = poly.ptr.polies;
		fprintf(fp,"(");
		while(size--){
			polyPrint(*head++,fp);
			if(size != 0){
				fprintf(fp," , ");
			}
		}
		fprintf(fp," )");
		return;
	}
	int first = 1;
	size_t index = 0;
	while(size-- > 0){
		//const Item item = *(poly.ptr.items++);
		const Item item = poly.ptr.items[index++];
		if(!cmpK(item.coefficient,K_0)){
			if(item.size == 0){
				fprintf(fp,"0");
			}else{
				fprintf(fp,"0\\cdot"); //Actually, this should not happen.
				DIE;
			}
			continue;
		}else if((!cmpK(item.coefficient,K_1) ||!cmpK(item.coefficient,K_N1)
					) && item.size > 0 ){
			if(!first && !cmpK(item.coefficient,K_1)){
				fprintf(fp,"+ ");
			}else if(!cmpK(item.coefficient,K_N1)){
				fprintf(fp,"- ");
			}else if( first ){
				;
			}else{
				DIE;
			}
			first = 0;
		}else{
			char buff[128] = {0};
			if(first){
				first = 0;
				fprintf(fp,"%s ",K2str(item.coefficient,buff));
			}else if(cmpK(item.coefficient,JOHO_NO_TANIGEN) < 0){
				K tmp = mulK(K_N1,item.coefficient);
				fprintf(fp," - %s ",K2str(tmp,buff));
			}else{
				fprintf(fp," + %s ",K2str(item.coefficient,buff));
			}
		}
		int i;
		for(i = 0; i < item.size;i++){
			if(item.degrees[i]){
				fprintf(fp,"x_{%d}^{%ld} ",i,item.degrees[i]);
			}
		}
	}
}
Poly item2Poly(mut Item item){
	Poly retval = {.size = 0};
	retval.ptr.items = malloc(sizeof(Item)*2);
	setPolySize(retval,1);
	setPolyType(retval,LEX);
	retval.ptr.items[0].degrees = item.degrees;
	retval.ptr.items[0].size = item.size;
	memcpy(&retval.ptr.items[0].coefficient,&item.coefficient,sizeof(K));
	//retval.ptr.items[0].coefficient = item.coefficient;
	//memcpy(retval.ptr.items,&item,sizeof(Item));
	return retval;
}
Item __polyIn(unmut Poly poly){
	if(polySize(poly) <= 0){
		fprintf(stderr,"This poly has no items\n");
		DIE;
	}
	return poly.ptr.items[0];
}
Item _polyIn(unmut Poly poly){
	return _copyItem(__polyIn(poly));
}
Poly polyIn(unmut Poly poly){
	Poly retval = item2Poly(_polyIn(poly));
	MonomialOrder order = polyType(poly);
	setPolyType(retval,order);
	return retval;
}
Poly polyAdd(unmut Poly v1,unmut Poly v2){
	if(polyType(v1) != polyType(v2)){
		fprintf(stderr,"Trying to add Poly sorted by different monomial order\n");
		DIE;
	}
	if(isNullPoly(v1) || isNullPoly(v2)){
		DIE;
		return nullPoly;
	}
	int (*cmp)(const Item *v1,const Item *v2) = NULL;
	switch(polyType(v1)){
		case LEX : {cmp = _polycmp_LEX;break;}
		case RLEX : {cmp = _polycmp_RLEX;break;}
		case PLEX : {cmp = _polycmp_PLEX;break;}
		case ARRAY : {DIE;}
	}
	int i,j,index;
	i = j = index = 0;
	Poly retval = {
		.ptr.items = malloc(sizeof(Item) * (polySize(v1) + polySize(v2)))
	};
	setPolySize(retval,(polySize(v1) + polySize(v2)));
	setPolyType(retval,polyType(v1));
	int someThingDisappeared = 0;
	while(i < polySize(v1) || j < polySize(v2)){
		int cmpVal;
		if(i >= polySize(v1)){
			cmpVal = -1;
		}else if(j >= polySize(v2)){
			cmpVal = +1;
		}else{
			cmpVal = cmp(&v1.ptr.items[i],&v2.ptr.items[j]);
		}
		if(cmpVal > 0){
			retval.ptr.items[index++] = _copyItem(v1.ptr.items[i++]);
		}else if(cmpVal < 0){
			retval.ptr.items[index++] = _copyItem(v2.ptr.items[j++]);
		}else{
			retval.ptr.items[index] = _copyItem(v1.ptr.items[i++]);
			retval.ptr.items[index].coefficient = addK(retval.ptr.items[index].coefficient
														, v2.ptr.items[j++].coefficient);
			if(!cmpK(retval.ptr.items[index].coefficient,K_0)){
				someThingDisappeared = 1;
				free(retval.ptr.items[index].degrees);
			}else{
				index++;
			}
			
		}
	}
	if(index == 0 && someThingDisappeared){
		index = 1;
		setPolySize(retval,index);
		retval.ptr.items = realloc(retval.ptr.items,sizeof(Item)*2);
		retval.ptr.items[0].degrees = NULL;
		retval.ptr.items[0].size = 0;
		retval.ptr.items[0].coefficient = K_0;
	}else if(index < polySize(retval)){
		retval.ptr.items = realloc(retval.ptr.items,sizeof(Item)*index);
		setPolySize(retval,index);
	}
	Poly tmp = polySort(retval,polyType(retval));polyFree(retval);
	return tmp;
}
Poly polySub(unmut Poly v1,unmut Poly v2){
	int i;
	for(i = 0;i < polySize(v2);i++){
		v2.ptr.items[i].coefficient = mulK(v2.ptr.items[i].coefficient,K_N1);
	}
	Poly retval = polyAdd(v1,v2);
	for(i = 0;i < polySize(v2);i++){
		v2.ptr.items[i].coefficient = mulK(v2.ptr.items[i].coefficient,K_N1);
	}
	return retval;
}
Poly polyMul(unmut Poly v1,unmut Poly v2){
	if(polyType(v1) != polyType(v2)){
		fprintf(stderr,"Trying to multiply Poly sorted by different monomial order\n");
		DIE;
	}
	if(isNullPoly(v1) || isNullPoly(v2)){
		DIE;
		return nullPoly;
	}
	int i,j,k,index;
	if(polySize(v1) == 0 || polySize(v2) == 0){
		fprintf(stderr,"FUCK\n");
		DIE;
	}
	Poly retval = {
		.size = 0,
	};
	retval.ptr.items = malloc(sizeof(Item)*(polySize(v1)*polySize(v2) + 1)); 
	index = 0;
	for(i = 0;i < polySize(v1);i++){
		for(j = 0;j < polySize(v2);j++){
			if(v1.ptr.items[i].size > v2.ptr.items[j].size){
				retval.ptr.items[index] = _copyItem(v1.ptr.items[i]);
				for(k = 0;k < v2.ptr.items[j].size ;k++){
					retval.ptr.items[index].degrees[k] += v2.ptr.items[j].degrees[k];
				}
				retval.ptr.items[index].coefficient = mulK(retval.ptr.items[index].coefficient, v2.ptr.items[j].coefficient);
			}else{
				retval.ptr.items[index] = _copyItem(v2.ptr.items[j]);
				for(k = 0;k < v1.ptr.items[i].size ;k++){
					retval.ptr.items[index].degrees[k] += v1.ptr.items[i].degrees[k];
				}
				retval.ptr.items[index].coefficient = mulK(retval.ptr.items[index].coefficient, v1.ptr.items[i].coefficient);
			}
			index++;			
		}
	}
	MonomialOrder order = polyType(v1);
	setPolySize(retval,index);
	setPolyType(retval,order);
	Poly tmp = retval;
	retval = polySort(tmp,order);
	if(isNullPoly(retval)){
		DIE;
	}
	polyFree(tmp);
	return retval;
}

int _isDiviable(unmut Item dividend,unmut Item divisor){
	if(dividend.size <  divisor.size){
		return 0;
	}
	size_t i;
	for(i = 0;i < divisor.size;i++){
		if(dividend.degrees[i] < divisor.degrees[i]){
			return 0;
		}
	}
	return 1;
}

/*This function returns array of Poly whose length is (size + 1).*/
/*divisor must be array of Poly whose length is 'size'*/
Poly polyDiv(unmut Poly dividend,unmut Poly divisors){
	int i,j,k;
	MonomialOrder order = polyType(dividend);
	size_t size = polySize(divisors);
	Poly h = polyDup(dividend);
	Item *in_g = malloc(sizeof(Item) * (size + 1));
	Poly *divisor;
	if(polyType(divisors) == ARRAY){
		divisor = unwrapPolyArray(divisors);
	}else{
		divisor = &divisors;
		size = 1;
	}
	for(i = 0;i < size;i++){
		in_g[i] = __polyIn(divisor[i]);
	}
	Poly *retval = malloc(sizeof(Poly)*(size + 1));
	for(i = 0;i < size;i++){
		setPolySize(retval[i],1);
		setPolyType(retval[i],order);
		retval[i].ptr.items = malloc(sizeof(Item)*2);
		memcpy(&retval[i].ptr.items[0].coefficient,&K_0,sizeof(K));
		retval[i].ptr.items[0].size = 0;
		retval[i].ptr.items[0].degrees = NULL;	
	}
	int exists;
	while(1){
		exists = 0;
		for(i = 0;i < size;i++){
			for(j = 0;j < polySize(h);j++){
				if(cmpK(in_g[i].coefficient,K_0) && _isDiviable(h.ptr.items[j],in_g[i])){
					exists = 1;
					unmut Item u = h.ptr.items[j];
					unmut Item g = in_g[i];
					K c_gi = g.coefficient;
					K c_f = u.coefficient;
					Item w = {
						.size = u.size,
						.degrees = malloc(sizeof(N)*u.size),
						.coefficient = divK(c_f , c_gi)
					};
					for(k = 0;k < g.size;k++){
						w.degrees[k] = u.degrees[k] - g.degrees[k];
					}
					for(;k < u.size;k++){
						w.degrees[k] = u.degrees[k];
					}
					for(;k > 0;k--){
						if(w.degrees[k-1]){
							break;
						}
					}
					if(w.size != k){
						w.size = k;
						w.degrees = realloc(w.degrees,sizeof(N)*k);
					}
					Poly tmp = item2Poly(w);
					setPolyType(tmp,order);
					Poly temp = polyMul(tmp,divisor[i]);
					//polyPrint(h,stderr);fprintf(stderr,"\n");
					h = polySort(h,polyType(h));
					Poly tempo = polySub(h,temp);polyFree(temp);polyFree(h);
					h = tempo;
					Poly newVal = polyAdd(retval[i],tmp);
					polyFree(retval[i]);
					retval[i] = newVal;
					polyFree(tmp);
					break;
					//polyPrint(h,stdout);
					//printf("\n");
				}
			}
		}
		if(!exists){
			break;
		}
	};
	retval[size] = h;
	return mkPolyArray(retval,size+1);
}

Poly polySim(unmut Poly dividend,unmut Poly divisors){
	Poly result = polyDiv(dividend,divisors);
	size_t size = polySize(result);
	Poly ret = polyDup(result.ptr.polies[size-1]);
	polyFree(result);
	return ret;
}

#define MAX(x,y,index) ((index >= x.size) ? y.degrees[index] : ( \
						(index >= y.size) ? x.degrees[index] : (\
						(x.degrees[index] > y.degrees[index]) ? x.degrees[index] : y.degrees[index])))

Poly polyS(unmut Poly f,unmut Poly g){
	MonomialOrder order = polyType(f);
	if(order != polyType(g)){
		fprintf(stderr,"S polynomial of different polynomials cannnot be computed\n");
		DIE;
	}
	Item in_f = _polyIn(f);
	Item in_g = _polyIn(g);
	Item w_f = {
		.size = (in_f.size < in_g.size) ? in_g.size : in_f.size,
		.coefficient = JOHO_NO_TANIGEN,
	};
	w_f.degrees = malloc(sizeof(N)*w_f.size);
	Item w_g = {
		.size = (in_f.size < in_g.size) ? in_g.size : in_f.size,
		.coefficient = JOHO_NO_TANIGEN
	};
	w_g.degrees = malloc(sizeof(N)*w_g.size);
	
	size_t i;
	for(i = 0;i < in_f.size;i++){
		w_f.degrees[i] = MAX(in_f,in_g,i) - in_f.degrees[i];
	}
	for(;i < w_f.size;i++){
		w_f.degrees[i] = MAX(in_f,in_g,i);
	}
	for(;i > 0;i--){
		if(w_f.degrees[i-1]){
			break;
		}
	}
	w_f.size = i;
	for(i = 0;i < in_g.size;i++){
		w_g.degrees[i] = MAX(in_f,in_g,i) - in_g.degrees[i];
	}
	for(;i < w_g.size;i++){
		w_g.degrees[i] = MAX(in_f,in_g,i);
	}
	for(;i > 0;i--){
		if(w_g.degrees[i-1]){
			break;
		}
	}
	w_g.size = i;
	w_f.coefficient = divK(K_1,in_f.coefficient);
	w_g.coefficient = divK(K_1,in_g.coefficient);
	Poly w_f_p = item2Poly(w_f);
	Poly w_g_p = item2Poly(w_g);
	setPolyType(w_f_p,order);
	setPolyType(w_g_p,order);
	Poly tmp_f = polyMul(w_f_p,f); polyFree(w_f_p);
	Poly tmp_g = polyMul(w_g_p,g); polyFree(w_f_p);
	Poly retval = polySub(tmp_f,tmp_g); polyFree(tmp_f);polyFree(tmp_g);
	return retval;
}

Item _copyItem(unmut Item item){
	Item retval = {
		.size = item.size,
		.coefficient = item.coefficient
	};
	if(item.size){
		retval.degrees = malloc(item.size * sizeof(N));
		memcpy(retval.degrees,item.degrees,item.size * sizeof(N));
	}else{
		retval.degrees = NULL;
	}
	return retval; 
}

Poly appendItem2Poly(mut Poly poly,mut Item item){
	size_t newSize = polySize(poly) + 1 ; // setPolySize is macro. To make it work as we want, this line is necessary
	setPolySize(poly, newSize );
	poly.ptr.items = realloc(poly.ptr.items,sizeof(Item)*newSize);
	poly.ptr.items[newSize - 1] = item;
	return poly;
}

int _polycmp_LEX(const Item *v1,const Item *v2){
	N degree_v1 = 0;
	N degree_v2 = 0;
	size_t i;
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
int _polycmp_RLEX(const Item *v1,const Item *v2){
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
int _polycmp_PLEX(const Item *v1,const Item *v2){
	int i;
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
int polyCMP_LEX(const Item *v1,const Item *v2){
	return -1*_polycmp_LEX(v1,v2);
}
int polyCMP_RLEX(const Item *v1,const Item *v2){
	return -1*_polycmp_RLEX(v1,v2);
}
int polyCMP_PLEX(const Item *v1,const Item *v2){
	return -1*_polycmp_PLEX(v1,v2);
}

int _isSameMonomial(unmut Item v1,unmut Item v2){
	if(v1.size != v2.size){
		return 0;
	}
	int i;
	for(i=0;i < v1.size;i++){
		if(v1.degrees[i] != v2.degrees[i]){
			return 0;
		}
	}
	return 1;
}

Poly polySort(unmut Poly poly,MonomialOrder order){
	Poly toBeSorted = polyDup(poly);
	switch(order){
		case LEX : {qsort(toBeSorted.ptr.items,polySize(toBeSorted),sizeof(Item)
					,(int (*)(const void *,const void *))polyCMP_LEX); setPolyType(toBeSorted,LEX); break;}
		case RLEX : {qsort(toBeSorted.ptr.items,polySize(toBeSorted),sizeof(Item)
					,(int (*)(const void *,const void *))polyCMP_RLEX); setPolyType(toBeSorted,RLEX); break;}
		case PLEX : {qsort(toBeSorted.ptr.items,polySize(toBeSorted),sizeof(Item)
					,(int (*)(const void *,const void *))polyCMP_PLEX); setPolyType(toBeSorted,PLEX); break;}
		default   : { DIE;}
	}
	Poly retval = {
		.size = toBeSorted.size, /*Copy entire toBeSorted.size. That is, both type of monomial order and size of *items*/
		.ptr.items = malloc(sizeof(Item) * polySize(toBeSorted))
	};
	int index = 0;
	int someThingDisappeared = 0;
	int i,j;
	for(i = 0;i < polySize(toBeSorted);i++){
		retval.ptr.items[index] = toBeSorted.ptr.items[i];
		for(j = i + 1;j < polySize(toBeSorted) && _isSameMonomial(toBeSorted.ptr.items[i],toBeSorted.ptr.items[j]);j++){
			retval.ptr.items[index].coefficient = addK(retval.ptr.items[index].coefficient, toBeSorted.ptr.items[j].coefficient);
			free(toBeSorted.ptr.items[j].degrees);
		}
		if(!cmpK(retval.ptr.items[index].coefficient , K_0)){
			free(retval.ptr.items[index].degrees);
			index--;
			someThingDisappeared = 1;
		}
		i = j - 1;
		index++;
	}
	if(index == 0 && someThingDisappeared){
		index = 1;
		setPolySize(retval,index);
		retval.ptr.items = realloc(retval.ptr.items,sizeof(Item)*2);
		retval.ptr.items[0].degrees = NULL;
		retval.ptr.items[0].size = 0;
		retval.ptr.items[0].coefficient = K_0;
	}else if(polySize(retval) != index){
		setPolySize(retval,index);
		retval.ptr.items = realloc(retval.ptr.items,sizeof(Item)*(index+1));
	}
	return retval;
}
Poly mkPolyArray(mut Poly *array,size_t size){
	Poly retval;
	retval.size = 0;
	setPolySize(retval,size);
	setPolyType(retval,ARRAY);
	retval.ptr.polies = array;
	return retval;
}
Poly * unwrapPolyArray(mut Poly poly){
	if(polyType(poly)!=ARRAY){
		return NULL;
	}
	return poly.ptr.polies;
}
int _isCoprime(Item v1,Item v2){
	size_t s = v1.size > v2.size ? v2.size : v1.size;
	size_t i;
	for(i = 0;i < s;i++){
		if(v1.degrees[i] && v2.degrees[i]){
			return 0;
		}
	}
	return 1;
}
Poly removeNullPolyFromArray(mut Poly poly){
	size_t i;
	size_t size = polySize(poly);
	Poly *ptr = unwrapPolyArray(poly);
	Poly *newPtr = malloc(sizeof(Poly)*size);
	size_t index = 0;
	for(i = 0;i < size;i++){
		if(!isNullPoly(ptr[i])){
			newPtr[index++] = ptr[i];
		}
	}
	free(ptr);
	if(index != size){
		size = index;
		newPtr =  realloc(newPtr,sizeof(Poly)*size);
	}
	setPolySize(poly,size);
	poly.ptr.polies = newPtr;
	return poly;
}
/*Returns 0 if array is grobner basis*/
/*Returns reminder of S(g_i,g_j) divided by array*/
Poly isThisGrobnerBasis(Poly array){
	size_t s = polySize(array);
	Poly *ptr = unwrapPolyArray(array);
	size_t i,j;
	Poly reminder;
	for(i = 0;i < s;i++){
		for(j = i + 1;j < s;j++){
			//if(_isCoprime(__polyIn(ptr[i]),__polyIn(ptr[j]))){
				//continue;
			//}
			Poly s_i_j = polyS(ptr[i],ptr[j]);
			reminder = polySim(s_i_j,array);polyFree(s_i_j);
			if(isZeroPoly(reminder)){
				polyFree(reminder);
			}else{
				goto ret;
			}
			Poly s_j_i = polyS(ptr[j],ptr[i]);
			reminder = polySim(s_j_i,array);polyFree(s_j_i);
			if(isZeroPoly(reminder)){
				polyFree(reminder);
			}else{
				goto ret;
			}
		}
	}
	Item item = {
		.size = 1,
		.coefficient = K_0,
		.degrees = NULL
	};
	reminder = item2Poly(item);
	MonomialOrder order = polyType(ptr[0]);
	setPolyType(reminder,order);
	ret:
	return reminder;
}
Poly GrobnerBasis2ReducedGrobnerBasis(mut Poly grobner){
	size_t i;
	size_t size = polySize(grobner);
	Poly *ptr = unwrapPolyArray(grobner);
	for(i = 0;i < size;i++){
		size_t j;
		for(j = 0; j < size;j++){
			if(i == j){
				continue;
			}else if(isNullPoly(ptr[j])){
				continue;
			}
			if(_isDiviable(__polyIn(ptr[i]),__polyIn(ptr[j]))){
				polyFree(ptr[i]);
				ptr[i] = nullPoly;
				break;
			}
		}
		if(!isNullPoly(ptr[i])){
			K gyaku = divK(K_1,ptr[i].ptr.items[0].coefficient);
			for(j = 0;j < polySize(ptr[i]);j++){
				ptr[i].ptr.items[j].coefficient = mulK(gyaku,ptr[i].ptr.items[j].coefficient);
			}
		}
	}
	grobner = removeNullPolyFromArray(grobner);
	int somethingHappened;
	do{
		somethingHappened = 0;
		ptr = unwrapPolyArray(grobner);
		size = polySize(grobner);
		ptr = realloc(ptr,sizeof(Poly)*size * 2);
		for(i = 0;i < size;i++){
			Poly _divisors = mkPolyArray(&ptr[i+1],size - 1);
			Poly divisors = removeNullPolyFromArray(polyDup(_divisors));
			Poly _result = polyDiv(ptr[i],divisors); polyFree(divisors);
			Poly *result = unwrapPolyArray(_result);
			size_t r_index = polySize(_result) - 1;
			Poly r = result[r_index];
			if(isZeroPoly(r)){
				polyFree(r);
				r = nullPoly;
			}else{
				size_t j;
				for(j = 0;j < r_index;j++){
					if(!isZeroPoly(result[j])){
						somethingHappened = 1;
						break;
					}
				}
				polyFree(ptr[i]);
				ptr[i] = nullPoly; 
			}
			ptr[i + size] = r;
		}
		size *= 2;
		setPolySize(grobner,size);
		grobner.ptr.polies = ptr;
		grobner = removeNullPolyFromArray(grobner);
	}while(somethingHappened);
	
	ptr = unwrapPolyArray(grobner);
	size = polySize(grobner);
	for(i = 0;i < size;i++){
		Poly p = ptr[i];
		size_t j;
		K gyaku = divK(K_1,p.ptr.items[0].coefficient);
		for(j = 0;j < polySize(p);j++){
			p.ptr.items[j].coefficient = mulK(gyaku,p.ptr.items[j].coefficient);
		}
	}
	return grobner;
}

