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
/*
size_t polySize(unmut Poly poly){return poly.size & 0xfffffffffffffff;}
MonomialOrder polyType(unmut Poly p){
	return (p.size >> 60) == MONOMIAL_ORDER_IN_BIN__RLEX ? RLEX : ((p.size >> 60)== MONOMIAL_ORDER_IN_BIN__PLEX ? PLEX : LEX);
}
void setPolySize(unmut Poly p,size_t newSize){
	p.size &= ((int64_t)0xf) << 60;
	p.size |= (0xfffffffffffffff & newSize);
}
void setPolyType(unmut Poly p,MonomialOrder t){
	p.size &= 0xfffffffffffffff;
	if( t == RLEX ){
		p.size |= (int64_t)MONOMIAL_ORDER_IN_BIN__RLEX << 60 ;
	}else if(t == PLEX){
		p.size |= (int64_t)MONOMIAL_ORDER_IN_BIN__PLEX << 60;
	}else{
		p.size |= (int64_t)MONOMIAL_ORDER_IN_BIN__LEX << 60;
	}
}
*/



extern int _isSameMonomial(unmut Item v1,unmut Item v2);
extern int _polycmp_LEX(const Item *v1,const Item *v2);
extern int _polycmp_RLEX(const Item *v1,const Item *v2);
extern int _polycmp_PLEX(const Item *v1,const Item *v2);
extern Item _copyItem(unmut Item item);

Poly polyDup(unmut Poly poly){
	Poly retval = {
		.size = poly.size,
		.items = malloc(sizeof(Item)*polySize(poly))
	};
	int i;
	for(i = 0;i < retval.size;i++){
		retval.items[i] = _copyItem(poly.items[i]);
	}
	return retval;
}

double poly2Double(unmut Poly poly){
	int i;
	double val = 0;
	Poly dup = polyDup(poly);
	for(i = 0;i < polySize(dup);i++){
		if(dup.items[i].size > 0){
			fprintf(stderr,"Trying to convert Poly to double but Poly has variable\n");
			DIE;
		}
		val += poly.items[i].coefficient;
	}
	polyFree(dup);
	return val;
}

void polyPrint(unmut Poly poly,FILE *fp){
	int first = 1;
	while(poly.size-- > 0){
		const Item item = *(poly.items++);
		if(item.coefficient == 0){
			continue;
		}else{
			if(first){
				first = 0;
				fprintf(stderr,"%.4f ",item.coefficient);
			}else if(item.coefficient < 0){
				fprintf(fp,"- %.4f ",fabs(item.coefficient));
			}else{
				fprintf(fp,"+ %.4f ",fabs(item.coefficient));
			}
		}
		int i;
		for(i = 0; i < item.size;i++){
			if(item.degrees[i]){
				fprintf(stderr,"x_{%d}^{%ld} ",i,item.degrees[i]);
			}
		}
	}
}

Item _polyIn(unmut Poly poly){
	if(polySize(poly) <= 0){
		fprintf(stderr,"This poly has no items\n");
		DIE;
	}
	return _copyItem(poly.items[0]);
}
Poly polyIn(unmut Poly poly){
	Item item = _polyIn(poly);
	Poly retval = {
		.size = 0,
		.items = malloc(sizeof(Poly))
	};
	retval.items[0] = item;
	setPolySize(retval,1);
	setPolyType(retval,polyType(poly));
	return retval;
}

Poly polyAdd(unmut Poly v1,unmut Poly v2){
	if(polyType(v1) != polyType(v2)){
		fprintf(stderr,"Trying to add Poly sorted by different monomial order\n");
		DIE;
	}
	int (*cmp)(const Item *v1,const Item *v2) = NULL;
	switch(polyType(v1)){
		case LEX : {cmp = _polycmp_LEX;break;}
		case RLEX : {cmp = _polycmp_RLEX;break;}
		case PLEX : {cmp = _polycmp_PLEX;break;}
	}
	int i,j,index;
	i = j = index = 0;
	Poly retval = {
		.size = 0,
		.items = malloc(sizeof(Item) * (polySize(v1) + polySize(v2)))
	};
	setPolySize(retval,(polySize(v1) + polySize(v2)));
	setPolyType(retval,polyType(v1));
	while(i < polySize(v1) && j < polySize(v2)){
		int cmpVal = cmp(&v1.items[i],&v2.items[j]);
		if(i >= polySize(v1)){
			cmpVal = 1;
		}else if(j >= polySize(v2)){
			cmpVal = -1;
		}
		if(cmpVal < 0){
			retval.items[index++] = _copyItem(v1.items[i++]);
		}else if(cmpVal > 0){
			retval.items[index++] = _copyItem(v2.items[j++]);
		}else{
			retval.items[index] = _copyItem(v1.items[i++]);
			retval.items[index++].coefficient += v2.items[j++].coefficient;
		}
	}
	if(index < polySize(retval)){
		retval.items = realloc(retval.items,sizeof(Item)*index);
	}
	return retval;
}
Poly polySub(unmut Poly v1,unmut Poly v2){
	int i;
	for(i = 0;i < polySize(v2);i++){
		v2.items[i].coefficient *= -1;
	}
	Poly retval = polyAdd(v1,v2);
	for(i = 0;i < polySize(v2);i++){
		v2.items[i].coefficient *= -1;
	}
	return retval;
}
Poly polyMul(unmut Poly v1,unmut Poly v2){
	if(polyType(v1) != polyType(v2)){
		fprintf(stderr,"Trying to multiply Poly sorted by different monomial order\n");
		DIE;
	}
	int i,j,k,index;
	Poly retval = {
		.size = 0,
		.items = malloc(sizeof(Item)*polySize(v1)*polySize(v2))
	};
	index = 0;
	for(i = 0;i < polySize(v1);i++){
		for(j = 0;j < polySize(v2);j++){
			if(v1.items[i].size > v2.items[j].size){
				retval.items[index] = _copyItem(v1.items[i]);
				for(k = 0;k < v2.items[j].size ;k++){
					retval.items[index].degrees[k] += v2.items[j].degrees[k];
				}
				retval.items[index].coefficient *= v2.items[j].coefficient;
			}else{
				retval.items[index] = _copyItem(v2.items[i]);
				for(k = 0;k < v1.items[j].size ;k++){
					retval.items[index].degrees[k] += v1.items[j].degrees[k];
				}
				retval.items[index].coefficient *= v1.items[j].coefficient;
			}
		}
	}
	MonomialOrder order = polyType(v1);
	Poly tmp = retval;
	retval = polySort(retval,order);
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
Poly * polyDiv(unmut Poly dividend,unmut Poly *divisor,unmut size_t size){
	int i,j,k;
	MonomialOrder order = polyType(dividend);
	Poly h = polyDup(dividend);
	Item *in_g = malloc(sizeof(Item) * size);
	for(i = 0;i < size;i++){
		in_g[i] = _polyIn(divisor[i]);
	}
	Poly *retval = malloc(sizeof(Poly)*(size + 1));
	for(i = 0;i < size;i++){
		setPolySize(retval[i],1);
		setPolyType(retval[i],order);
		retval[i].items = malloc(sizeof(Item));
		retval[i].items[0].coefficient = 0;
		retval[i].items[0].size = 0;
		retval[i].items[0].degrees = NULL;	
	}
	int exists;
	while(1){
		exists = 0;
		for(i = 0;i < size;i++){
			for(j = 0;j < polySize(h);j++){
				if(_isDiviable(h.items[j],in_g[i])){
					exists = 1;
					unmut Item u = h.items[j];
					unmut Item g = in_g[i];
					K c_gi = g.coefficient;
					K c_f = u.coefficient;
					Item w = {
						.size = u.size,
						.degrees = malloc(sizeof(N)*u.size),
						.coefficient = c_f / c_gi
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
					Poly tmp = {
						.items = malloc(sizeof(Item))
					};
					tmp.items[0] = w;
					setPolySize(tmp,1);
					setPolyType(tmp,order);
					Poly temp = polyMul(tmp,divisor[i]);
					free(tmp.items);
					Poly tempo = polySub(h,temp);
					polyFree(temp);
					polyFree(h);
					h = tempo;
					if(retval[i].size == 1 && retval[i].items[0].size == 0 && retval[i].items[0].coefficient == 0){
						/*NEW!!*/
						retval[i].items[0] = w;
					}else{
						/*already here*/
						Poly newVal = appendItem2Poly(retval[i],w);
						retval[i] = polySort(newVal,order);
						polyFree(newVal);
					}
				}
			}
		}
		if(!exists){
			break;
		}
	}while(1);
	retval[size] = h;
	return retval;
}

void polyFree(mut Poly v){
	int i;
	for(i=0;i < polySize(v);i++){
		free(v.items[i].degrees);
	}
	free(v.items);
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

Poly appendItem2Poly(mut Poly poly,mut Item item){
	fprintf(stderr,"Poly size :%ld\n",polySize(poly));
	size_t newSize = polySize(poly) + 1 ; // setPolySize is macro. To make it work as we want, this line is necessary
	setPolySize(poly, newSize );
	fprintf(stderr,"Poly size changed : %ld\n",polySize(poly));
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
					,(int (*)(const void *,const void *))_polycmp_LEX); setPolyType(toBeSorted,LEX); break;}
		case RLEX : {qsort(toBeSorted.items,polySize(toBeSorted),sizeof(Item)
					,(int (*)(const void *,const void *))_polycmp_RLEX); setPolyType(toBeSorted,RLEX); break;}
		case PLEX : {qsort(toBeSorted.items,polySize(toBeSorted),sizeof(Item)
					,(int (*)(const void *,const void *))_polycmp_PLEX); setPolyType(toBeSorted,PLEX); break;}
	}
	Poly retval = {
		.size = toBeSorted.size, /*Copy entire toBeSorted.size. That is, both type of monomial order and size of *items*/
		.items = malloc(sizeof(Item) * polySize(toBeSorted))
	};
	int index = 0;
	int i,j;
	for(i = 0;i < polySize(toBeSorted);i++){
		retval.items[index] = toBeSorted.items[i];
		for(j = i + 1;j < polySize(toBeSorted) && _isSameMonomial(toBeSorted.items[i],toBeSorted.items[j]);j++){
			retval.items[index].coefficient += toBeSorted.items[j].coefficient;
			free(toBeSorted.items[j].degrees);
		}
		i = j - 1;
		index++;
	}
	if(retval.size != index){
		setPolySize(retval,index);
		retval.items = realloc(retval.items,sizeof(Item)*polySize(retval));
	}
	return retval;
}

