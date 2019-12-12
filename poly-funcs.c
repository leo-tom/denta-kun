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
	.items = NULL
};

extern int _isSameMonomial(unmut Item v1,unmut Item v2);
extern int _polycmp_LEX(const Item *v1,const Item *v2);
extern int _polycmp_RLEX(const Item *v1,const Item *v2);
extern int _polycmp_PLEX(const Item *v1,const Item *v2);
extern Item _copyItem(unmut Item item);
void _polyFree(mut Poly v){
	int i;
	for(i=0;i < polySize(v);i++){
		free(v.items[i].degrees);
	}
	free(v.items);
}
void polyFree(mut Poly v){
	size_t size = polySize(v);
	size_t i;
	if(polyType(v) == ARRAY){
		Poly *ptr = (Poly *)v.items;
		for(i = 0;i < size;i++){
			polyFree(*ptr++);
		}
	}else{
		for(i=0;i < size;i++){
			free(v.items[i].degrees);
		}
	}
	if(size > 0){
		free(v.items);
	}
}
int isNullPoly(unmut Poly poly){
	return poly.size == nullPoly.size
			&& poly.items == nullPoly.items;
}
Poly polyDup(unmut Poly poly){
	Poly retval = {
		.size = poly.size
	};
	if(polyType(poly) == ARRAY){
		retval.items = malloc(sizeof(Poly)*polySize(poly));
		Poly *from = (Poly *)poly.items;
		Poly *to = (Poly *)retval.items;
		int i;
		for(i = 0;i < polySize(poly);i++){
			to[i] = polyDup(from[i]);
		}
	}else{
		retval.items = malloc(sizeof(Item)*polySize(poly));
		int i;
		for(i = 0;i < polySize(poly);i++){
			retval.items[i] = _copyItem(poly.items[i]);
		}
	}
	return retval;
}

double poly2Double(unmut Poly poly){
	if(poly.size != 1 || poly.items[0].size > 0){
		fprintf(stderr,"Trying to call poly2Double to : ");polyPrint(poly,stderr);fprintf(stderr,"\n");
		DIE;
	}
	return K2double(poly.items[0].coefficient);
}

void polyPrint(unmut Poly poly,FILE *fp){
	size_t size = polySize(poly);
	if(isNullPoly(poly)){
		fprintf(fp,"(null)");
		return;
	}else if(polyType(poly) == ARRAY){
		Poly *head = (Poly *)poly.items;
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
	while(size-- > 0){
		const Item item = *(poly.items++);
		if(!cmpK(item.coefficient,KAHO_NO_TANIGEN)){
			if(item.size == 0){
				fprintf(fp,"0");
			}else{
				fprintf(fp,"0\\cdot"); //Actually, this should not happen.
				DIE;
			}
			continue;
		}else if((!cmpK(item.coefficient,JOHO_NO_TANIGEN) ||!cmpK(item.coefficient,JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN)
					) && item.size > 0 ){
			if(!first && !cmpK(item.coefficient,JOHO_NO_TANIGEN)){
				fprintf(fp,"+ ");
			}else if(!cmpK(item.coefficient,JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN)){
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

Item __polyIn(unmut Poly poly){
	if(polySize(poly) <= 0){
		fprintf(stderr,"This poly has no items\n");
		DIE;
	}
	return poly.items[0];
}
Item _polyIn(unmut Poly poly){
	return _copyItem(__polyIn(poly));
}
Poly polyIn(unmut Poly poly){
	Item item = _polyIn(poly);
	Poly retval = {
		.size = 0,
		.items = sizeof(Poly) ? 0 : malloc(sizeof(Poly))
	};
	retval.items[0] = item;
	setPolySize(retval,1);
	setPolyType(retval,polyType(poly));
	return retval;
}

Poly item2Poly(mut Item item){
	Poly retval = {
		.items = malloc(sizeof(Item)*2)
	};
	setPolySize(retval,1);
	setPolyType(retval,LEX);
	retval.items[0] = item;
	return retval;
}	

Poly polyAdd(unmut Poly v1,unmut Poly v2){
	if(polyType(v1) != polyType(v2)){
		fprintf(stderr,"Trying to add Poly sorted by different monomial order\n");
		DIE;
	}
	if(isNullPoly(v1) || isNullPoly(v2)){
		//DIE;
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
		.items = malloc(sizeof(Item) * (polySize(v1) + polySize(v2)))
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
			cmpVal = cmp(&v1.items[i],&v2.items[j]);
		}
		if(/*cmpVal != 0 && */index > 0 && ! cmpK(retval.items[index-1].coefficient,K_0)){
			index--;
			free(retval.items[index].degrees);
			someThingDisappeared = 1;
		}
		if(cmpVal > 0){
			retval.items[index++] = _copyItem(v1.items[i++]);
		}else if(cmpVal < 0){
			retval.items[index++] = _copyItem(v2.items[j++]);
		}else{
			retval.items[index] = _copyItem(v1.items[i++]);
			retval.items[index].coefficient = addK(retval.items[index].coefficient
														, v2.items[j].coefficient);
			index++; j++;
		}
	}
	if(index > 0 && !cmpK( retval.items[index-1].coefficient , K_0)){
		index--;
		free(retval.items[index].degrees);
		someThingDisappeared = 1;
	}
	if(index == 0 && someThingDisappeared){
		index = 1;
		setPolySize(retval,index);
		retval.items = realloc(retval.items,sizeof(Item)*2);
		retval.items[0].degrees = NULL;
		retval.items[0].size = 0;
		retval.items[0].coefficient = K_0;
	}else if(index < polySize(retval)){
		retval.items = realloc(retval.items,sizeof(Item)*index);
		setPolySize(retval,index);
	}
	return retval;
}
Poly polySub(unmut Poly v1,unmut Poly v2){
	int i;
	for(i = 0;i < polySize(v2);i++){
		v2.items[i].coefficient = mulK(v2.items[i].coefficient,JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN);
	}
	Poly retval = polyAdd(v1,v2);
	for(i = 0;i < polySize(v2);i++){
		v2.items[i].coefficient = mulK(v2.items[i].coefficient,JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN);
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
	Poly retval = {
		.size = 0,
		.items = malloc(sizeof(Item)*(polySize(v1)*polySize(v2) + 1))
	};
	index = 0;
	for(i = 0;i < polySize(v1);i++){
		for(j = 0;j < polySize(v2);j++){
			if(v1.items[i].size > v2.items[j].size){
				retval.items[index] = _copyItem(v1.items[i]);
				for(k = 0;k < v2.items[j].size ;k++){
					retval.items[index].degrees[k] += v2.items[j].degrees[k];
				}
				retval.items[index].coefficient = mulK(retval.items[index].coefficient, v2.items[j].coefficient);
			}else{
				retval.items[index] = _copyItem(v2.items[j]);
				for(k = 0;k < v1.items[i].size ;k++){
					retval.items[index].degrees[k] += v1.items[i].degrees[k];
				}
				retval.items[index].coefficient = mulK(retval.items[index].coefficient, v1.items[i].coefficient);
			}
			index++;
		}
	}
	MonomialOrder order = polyType(v1);
	setPolySize(retval,index);
	setPolyType(retval,order);
	Poly tmp = retval;
	retval = polySort(retval,order);
	if(isNullPoly(retval)){
		fprintf(stderr,"FUCK\n");
		DIE;
	}
	polyFree(tmp);
	return retval;
}

int _isDiviable(unmut Item dividend,unmut Item divisor){
	if(dividend.size <  divisor.size){
		return 0;
	}
	int i;
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
	Item *in_g = malloc(sizeof(Item) * size);
	Poly *divisor;
	if(polyType(divisors) == ARRAY){
		divisor = unwrapPolyArray(divisors);
	}else{
		divisor = &divisors;
	}
	for(i = 0;i < size;i++){
		in_g[i] = __polyIn(divisor[i]);
	}
	Poly *retval = malloc(sizeof(Poly)*(size + 1));
	for(i = 0;i < size;i++){
		setPolySize(retval[i],1);
		setPolyType(retval[i],order);
		retval[i].items = malloc(sizeof(Item)*2);
		retval[i].items[0].coefficient = KAHO_NO_TANIGEN;
		retval[i].items[0].size = 0;
		retval[i].items[0].degrees = NULL;	
	}
	int exists;
	while(1){
		exists = 0;
		for(i = 0;i < size;i++){
			for(j = 0;j < polySize(h);j++){
				if(_isDiviable(h.items[0],in_g[i])){
					exists = 1;
					unmut Item u = h.items[0];
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
					for(;k >= 0;k--){
						if(w.degrees[k-1]){
							break;
						}
					}
					if(w.size != k){
						w.size = k;
						w.degrees = realloc(w.degrees,k);
					}
					Poly tmp = item2Poly(w);
					setPolyType(tmp,order);
					Poly temp = polyMul(tmp,divisor[i]);
					Poly tempo = polySub(h,temp);polyFree(temp);polyFree(h);
					h = tempo;
					Poly newVal = polyAdd(retval[i],tmp);
					polyFree(retval[i]);
					retval[i] = newVal;
					polyFree(tmp);
					polyPrint(h,stdout);
					printf("\n");
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
	size_t size = polySize(result) - 1;
	setPolySize(result,size);
	Poly ret = ((Poly *)result.items)[size];
	polyFree(result);
	return ret;
}

#define MAX(x,y,index) (index >= x.size) ? y.degrees[index] : ( \
						(index >= y.size) ? x.degrees[index] : (\
						(x.degrees[index] > y.degrees[index]) ? x.degrees[index] : y.degrees[index]))

Poly polyS(unmut Poly f,unmut Poly g){
	MonomialOrder order = polyType(f);
	if(order != polyType(g)){
		fprintf(stderr,"S polynomial of different polynomials cannnot be computed\n");
		DIE;
	}
	Item in_f = __polyIn(f);
	Item in_g = __polyIn(g);
	Item w_f = {
		.size = (in_f.size < in_g.size) ? in_g.size : in_f.size,
		.degrees = malloc(sizeof(N)*w_f.size),
		.coefficient = JOHO_NO_TANIGEN
	};
	Item w_g = {
		.size = (in_f.size < in_g.size) ? in_g.size : in_f.size,
		.degrees = malloc(sizeof(N)*w_g.size),
		.coefficient = JOHO_NO_TANIGEN
	};
	
	size_t i;
	for(i = 0;i < w_f.size;i++){
		w_f.degrees[i] = MAX(in_f,in_g,i) - in_f.degrees[i];
	}
	for(i = 0;i < w_g.size;i++){
		w_g.degrees[i] = MAX(in_f,in_g,i) - in_g.degrees[i];
	}
	w_f.coefficient = divK(JOHO_NO_TANIGEN,in_f.coefficient);
	w_g.coefficient = divK(JOHO_NO_TANIGEN,in_g.coefficient);
	Poly w_f_p = item2Poly(w_f);
	Poly w_g_p = item2Poly(w_g);
	setPolyType(w_f_p,order);
	setPolyType(w_g_p,order);
	Poly tmp_f = polyMul(w_f_p,f); polyFree(w_f_p);
	Poly tmp_g = polyMul(w_g_p,g); polyFree(w_f_p);
	Poly retval = polyAdd(tmp_f,tmp_g); polyFree(tmp_f);polyFree(tmp_g);
	return retval;
}

Item _copyItem(unmut Item item){
	Item retval = {
		.size = item.size,
		.coefficient = item.coefficient,
		.degrees = malloc(sizeof(item.size) * sizeof(N))
	};
	memcpy(retval.degrees,item.degrees,item.size * sizeof(N));
	return retval; 
}

Poly appendItem2Poly(mut Poly poly,mut Item item){
	size_t newSize = polySize(poly) + 1 ; // setPolySize is macro. To make it work as we want, this line is necessary
	setPolySize(poly, newSize );
	poly.items = realloc(poly.items,sizeof(Item)*polySize(poly));
	poly.items[polySize(poly) - 1] = item;
	return poly;
}

int _polycmp_LEX(const Item *v1,const Item *v2){
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
		case LEX : {qsort(toBeSorted.items,polySize(toBeSorted),sizeof(Item)
					,(int (*)(const void *,const void *))polyCMP_LEX); setPolyType(toBeSorted,LEX); break;}
		case RLEX : {qsort(toBeSorted.items,polySize(toBeSorted),sizeof(Item)
					,(int (*)(const void *,const void *))polyCMP_RLEX); setPolyType(toBeSorted,RLEX); break;}
		case PLEX : {qsort(toBeSorted.items,polySize(toBeSorted),sizeof(Item)
					,(int (*)(const void *,const void *))polyCMP_PLEX); setPolyType(toBeSorted,PLEX); break;}
		default   : { DIE;}
	}
	Poly retval = {
		.size = toBeSorted.size, /*Copy entire toBeSorted.size. That is, both type of monomial order and size of *items*/
		.items = malloc(sizeof(Item) * polySize(toBeSorted))
	};
	int index = 0;
	int someThingDisappeared = 0;
	int i,j;
	for(i = 0;i < polySize(toBeSorted);i++){
		retval.items[index] = toBeSorted.items[i];
		for(j = i + 1;j < polySize(toBeSorted) && _isSameMonomial(toBeSorted.items[i],toBeSorted.items[j]);j++){
			retval.items[index].coefficient = addK(retval.items[index].coefficient, toBeSorted.items[j].coefficient);
			free(toBeSorted.items[j].degrees);
		}
		if(!cmpK(retval.items[index].coefficient , KAHO_NO_TANIGEN)){
			free(retval.items[index].degrees);
			index--;
			someThingDisappeared = 1;
		}
		i = j - 1;
		index++;
	}
	if(index == 0 && someThingDisappeared){
		index = 1;
		setPolySize(retval,index);
		retval.items = realloc(retval.items,sizeof(Item)*2);
		retval.items[0].degrees = NULL;
		retval.items[0].size = 0;
		retval.items[0].coefficient = K_0;
	}else if(polySize(retval) != index){
		setPolySize(retval,index);
		retval.items = realloc(retval.items,sizeof(Item)*polySize(retval));
	}
	return retval;
}
Poly mkPolyArray(mut Poly *array,size_t size){
	Poly retval;
	retval.size = 0;
	setPolySize(retval,size);
	setPolyType(retval,ARRAY);
	retval.items = (Item *)array;
	return retval;
}
Poly * unwrapPolyArray(mut Poly poly){
	if(polyType(poly)!=ARRAY){
		return NULL;
	}
	return (Poly *)poly.items;
}
