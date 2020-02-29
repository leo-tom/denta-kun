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

const Poly nullPoly = {
	.size = 0,
	.ptr.items = NULL
};

Item _zeroItem[] = { {.size = 0,.degrees = 0}};
const Poly zeroPoly = {
	.size = 1,
	.ptr.items = _zeroItem
};

/*
S.<x,y,z> = PolynomialRing(QQ, 3, order='deglex')
J = Ideal([x^3 - 3*x^2 - y + 1, -x^2 + y^2 - 1])
J.groebner_basis()

*/

extern int _isSameMonomial(unmut Item v1,unmut Item v2);
extern int _polycmp_LEX(const Item *v1,const Item *v2);
extern int _polycmp_RLEX(const Item *v1,const Item *v2);
extern int _polycmp_PLEX(const Item *v1,const Item *v2);
extern Item _copyItem(unmut Item item);

void _setPolySize(Poly *poly,size_t size){
	poly->size &= ((int64_t)0xf) << 60;
	poly->size |= (0xfffffffffffffff & size);
}
void _setPolyType(Poly *poly,MonomialOrder order){
	poly->size &= 0xfffffffffffffff;
	if( order == RLEX ){
		poly->size |= (int64_t)MONOMIAL_ORDER_IN_BIN__RLEX << 60 ;
	}else if(order == PLEX){
		poly->size |= (int64_t)MONOMIAL_ORDER_IN_BIN__PLEX << 60;
	}else if(order == ARRAY){ 
		poly->size |= (int64_t)MONOMIAL_ORDER_IN_BIN__ARRAY << 60;
	}else{
		poly->size |= (int64_t)MONOMIAL_ORDER_IN_BIN__LEX << 60;
	}
}

Poly K2Poly(unmut K k,MonomialOrder order){
	Poly retval;
	setPolySize(retval,1);
	setPolyType(retval,order);
	retval.ptr.items = malloc(sizeof(Item));
	retval.ptr.items->size = 0;
	retval.ptr.items->degrees = NULL;
	copyK(retval.ptr.items->coefficient, k);
	return retval;
}

void polyFree(mut Poly v){
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
			freeK(v.ptr.items[i].coefficient);
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
N polyDegrees(unmut Poly p){
	N max = 0;
	size_t i,j;
	for(i = 0;i < polySize(p);i++){
		N val = 0;
		for(j = 0;j < p.ptr.items[i].size;j++){
			val += p.ptr.items[i].degrees[j];
		}
		if(val > max){
			max = val;
		}
	}
	return max;
}
Poly polyDup(unmut Poly poly){
	if(polySize(poly) == 0){
		return nullPoly;
	}
	Poly retval = {
		.size = poly.size
	};
	if(polyType(poly) == ARRAY){
		Poly *to = malloc(sizeof(poly)*polySize(poly));
		Poly *from = poly.ptr.polies;
		retval.ptr.polies = to;
		size_t i;
		for(i = 0;i < polySize(poly);i++){
			to[i] = polyDup(from[i]);
		}
	}else{
		retval.ptr.items = malloc(sizeof(Item)*polySize(poly));
		if(retval.ptr.items == NULL){
			DIE;
		}
		size_t i;
		for(i = 0;i < polySize(poly);i++){
			retval.ptr.items[i] = _copyItem(poly.ptr.items[i]);
		}
	}
	return retval;
}

double poly2Double(unmut Poly poly){
	if(poly.size != 1 || poly.ptr.items[0].size > 0){
		fprintf(stderr,"Trying to call poly2Double to : ");polyPrint(poly,K2str,stderr);fprintf(stderr,"\n");
		DIE;
	}
	return K2double(poly.ptr.items[0].coefficient);
}

void polyPrint(unmut Poly poly,char*(*printer)(K , char *),FILE *fp){
	size_t size = polySize(poly);
	if(isNullPoly(poly)){
		fprintf(fp,"(null)");
		return;
	}else if(polyType(poly) == ARRAY){
		Poly *head = poly.ptr.polies;
		fprintf(fp,"(");
		while(size--){
			polyPrint(*head,printer,fp);
			if(size != 0){
				fprintf(fp," , ");
			}
			if(polyType(*head) == ARRAY){
				fprintf(fp,"\n");
			}
			head++;
		}
		fprintf(fp," )");
		return;
	}
	int first = 1;
	size_t index = 0;
	while(size-- > 0){
		unmut Item item = poly.ptr.items[index++];
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
				#if BOOLEAN == 1
				if(!first) {
					fprintf(fp,"+ ");
				}
				#else
				fprintf(fp,"- ");
				#endif
			}else if( first ){
				;
			}else{
				DIE;
			}
		}else{
			char buff[128] = {0};
			if(first){
				fprintf(fp,"%s ",printer(item.coefficient,buff));
			}else if(cmpK(item.coefficient,JOHO_NO_TANIGEN) < 0){
				K tmp;
				initK(tmp);
				mulK(tmp,K_N1,item.coefficient);
				fprintf(fp," - %s ",printer(tmp,buff));
			}else{
				fprintf(fp," + %s ",printer(item.coefficient,buff));
			}
		}
		size_t i;
		for(i = 0; i < item.size;i++){
			if(item.degrees[i]){
				#if BOOLEAN
				fprintf(fp,"x_{%ld}^{%d} ",i+SUBSHIFT,item.degrees[i]);
				#else
				fprintf(fp,"x_{%ld}^{%ld} ",i+SUBSHIFT,item.degrees[i]);
				#endif
			}
		}
		first = 0;
	}
}
Poly item2Poly(mut Item item){
	Poly retval = {.size = 0};
	Item *_tmp = malloc(sizeof(Item));
	retval.ptr.items = _tmp;
	setPolySize(retval,1);
	setPolyType(retval,LEX);
	retval.ptr.items->degrees = item.degrees;
	retval.ptr.items->size = item.size;
	copyK(retval.ptr.items->coefficient, item.coefficient);
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
int cmpItem(MonomialOrder order,Item v1,Item v2){
	int (*cmp)(const Item *,const Item *) = NULL;
	switch(order){
		case LEX : {cmp = _polycmp_LEX;break;}
		case RLEX : {cmp = _polycmp_RLEX;break;}
		case PLEX : {cmp = _polycmp_PLEX;break;}
		default   : { DIE;}
	}
	return cmp(&v1,&v2);
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
		default   : { DIE;}
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
			addK(retval.ptr.items[index].coefficient,
					retval.ptr.items[index].coefficient
														, v2.ptr.items[j++].coefficient);
			if(!cmpK(retval.ptr.items[index].coefficient,K_0)){
				someThingDisappeared = 1;
				free(retval.ptr.items[index].degrees);
				freeK(retval.ptr.items[index].coefficient);
			}else{
				index++;
			}
			
		}
	}
	if(index == 0 && someThingDisappeared){
		retval.ptr.items = realloc(retval.ptr.items,sizeof(Item));
		retval.ptr.items[0].degrees = NULL;
		retval.ptr.items[0].size = 0;
		copyK(retval.ptr.items[0].coefficient,K_0);
		setPolySize(retval,1);
	}else if(index < polySize(retval)){
		retval.ptr.items = realloc(retval.ptr.items,sizeof(Item)*index);
		setPolySize(retval,index);
	}
	return retval;
}
Poly polySub(unmut Poly v1,unmut Poly v2){
	int i;
	for(i = 0;i < polySize(v2);i++){
		mulK(v2.ptr.items[i].coefficient,v2.ptr.items[i].coefficient,K_N1);
	}
	Poly retval = polyAdd(v1,v2);
	for(i = 0;i < polySize(v2);i++){
		mulK(v2.ptr.items[i].coefficient,v2.ptr.items[i].coefficient,K_N1);
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
				mulK(retval.ptr.items[index].coefficient,retval.ptr.items[index].coefficient, v2.ptr.items[j].coefficient);
			}else{
				retval.ptr.items[index] = _copyItem(v2.ptr.items[j]);
				for(k = 0;k < v1.ptr.items[i].size ;k++){
					retval.ptr.items[index].degrees[k] += v1.ptr.items[i].degrees[k];
				}
				mulK(retval.ptr.items[index].coefficient,retval.ptr.items[index].coefficient, v1.ptr.items[i].coefficient);
			}
			index++;			
		}
	}
	MonomialOrder order = polyType(v1);
	setPolySize(retval,index);
	setPolyType(retval,order);
	Poly tmp = retval;
	retval = polySort(tmp,order);
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
	Poly *retval = malloc(sizeof(Poly) * (size + 1));
	for(i = 0;i < size;i++){
		Item *ptr = malloc(sizeof(Item));
		copyK(ptr->coefficient,K_0);
		ptr->size = 0;
		ptr->degrees = NULL;
		retval[i].ptr.items = ptr;
		setPolySize(retval[i],1);
		setPolyType(retval[i],order);
	}
	while(1){
		for(i = 0;i < polySize(h);i++){
			for(j = 0;j < size;j++){
				if(_isDiviable(h.ptr.items[i],in_g[j])){
					goto found;
				}
			}
		}
		break;
		found : {
			Item u = h.ptr.items[i];
			Item g = in_g[j];
			Item w = {
				.size = u.size,
				.degrees = malloc(sizeof(N)*u.size)
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
			initK(w.coefficient);
			divK(w.coefficient,u.coefficient , g.coefficient);
			mulK(w.coefficient,w.coefficient , K_N1);
			Poly tmp = item2Poly(w);
			setPolyType(tmp,order);
			Poly temp = polyMul(tmp,divisor[j]);
			#if DEBUG >= 2
				fprintf(stderr,"h     : ");polyPrint(h,K2str,stderr);fprintf(stderr,"\n");
				fprintf(stderr,"wf_i  : ");polyPrint(temp,K2str,stderr);fprintf(stderr,"\n");
			#endif
			Poly tempo = polyAdd(h,temp);
			polyFree(h);
			polyFree(temp);
			#if DEBUG >= 2	
				fprintf(stderr,"h-wf_i: ");polyPrint(tempo,K2str,stderr);fprintf(stderr,"\n");fprintf(stderr,"\n");
			#endif
			h = tempo;
			Poly newVal = polyAdd(retval[j],tmp);
			polyFree(tmp);
			polyFree(retval[j]);
			retval[j] = newVal;
		}
	}
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
					
N __max(Item x,Item y,size_t index){
	if(index >= x.size && index >= y.size){
		DIE;
	}
	if(index >= x.size){
		return y.degrees[index];
	}else if(index >= y.size){
		return x.degrees[index];
	}else if(x.degrees[index] > y.degrees[index]){
		return x.degrees[index];
	}else{
		return y.degrees[index];
	}
}

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
	};
	Item w_g = {
		.size = (in_f.size < in_g.size) ? in_g.size : in_f.size,
	};
	w_g.degrees = malloc(sizeof(N)*w_g.size);
	w_f.degrees = malloc(sizeof(N)*w_f.size);
	initK(w_f.coefficient);
	initK(w_g.coefficient);
	size_t i;
	for(i = 0;i < in_f.size;i++){
		w_f.degrees[i] = __max(in_f,in_g,i) - in_f.degrees[i];
	}
	for(;i < w_f.size;i++){
		w_f.degrees[i] = __max(in_f,in_g,i);
	}
	for(;i > 0;i--){
		if(w_f.degrees[i-1]){
			break;
		}
	}
	w_f.size = i;
	for(i = 0;i < in_g.size;i++){
		w_g.degrees[i] = __max(in_f,in_g,i) - in_g.degrees[i];
	}
	for(;i < w_g.size;i++){
		w_g.degrees[i] = __max(in_f,in_g,i);
	}
	for(;i > 0;i--){
		if(w_g.degrees[i-1]){
			break;
		}
	}
	w_g.size = i;
	divK(w_f.coefficient,K_1,in_f.coefficient);
	divK(w_g.coefficient,K_1,in_g.coefficient);
	Poly w_f_p = item2Poly(w_f);
	Poly w_g_p = item2Poly(w_g);
	setPolyType(w_f_p,order);
	setPolyType(w_g_p,order);
	Poly tmp_f = polyMul(w_f_p,f); polyFree(w_f_p);
	Poly tmp_g = polyMul(w_g_p,g); polyFree(w_g_p);
	Poly retval = polySub(tmp_f,tmp_g); polyFree(tmp_f);polyFree(tmp_g);
	return retval;
}

Item _copyItem(unmut Item item){
	Item retval = {
		.size = item.size
	};
	copyK(retval.coefficient,item.coefficient);
	if(item.size){
		retval.degrees = malloc(item.size * sizeof(N));
		if(item.degrees == NULL){
			fprintf(stderr,"saa");
			DIE;
		}
		memcpy(retval.degrees,item.degrees,item.size * sizeof(N));
	}else{
		retval.degrees = NULL;
	}
	return retval; 
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

void polyNice(unmut Poly p){
	#if !RATIONAL
	return;
	#endif
	if(polySize(p) == 0 || !cmpK(p.ptr.items[0].coefficient,K_0)
			|| !cmpK(p.ptr.items[0].coefficient,K_1)){
		return;
	}
	size_t j;
	K gyaku;
	initK(gyaku);
	divK(gyaku,K_1,p.ptr.items[0].coefficient);
	for(j = 0;j < polySize(p);j++){
		mulK(p.ptr.items[j].coefficient,
		gyaku,p.ptr.items[j].coefficient);
	}
	freeK(gyaku);
}

Poly polySort(unmut Poly poly,MonomialOrder order){
	if(polyType(poly) == ARRAY){
		size_t size = polySize(poly);
		size_t i;
		Poly *source = unwrapPolyArray(poly);
 		Poly *ptr = malloc(size * sizeof(Poly));
		for(i = 0;i < size;i++){
			ptr[i] = polySort(source[i],order);
		}
		return mkPolyArray(ptr,size);
	}
	Poly toBeSorted = polyDup(poly);
	size_t size = polySize(toBeSorted);
	switch(order){
		case LEX : {qsort(toBeSorted.ptr.items,size,sizeof(Item)
					,(int (*)(const void *,const void *))polyCMP_LEX); setPolyType(toBeSorted,LEX); break;}
		case RLEX : {qsort(toBeSorted.ptr.items,size,sizeof(Item)
					,(int (*)(const void *,const void *))polyCMP_RLEX); setPolyType(toBeSorted,RLEX); break;}
		case PLEX : {qsort(toBeSorted.ptr.items,size,sizeof(Item)
					,(int (*)(const void *,const void *))polyCMP_PLEX); setPolyType(toBeSorted,PLEX); break;}
		default   : { DIE;}
	}
	Poly retval = {
		.size = toBeSorted.size, /*Copy entire toBeSorted.size. That is, both type of monomial order and size of *items*/
		.ptr.items = malloc(sizeof(*retval.ptr.items) * polySize(toBeSorted))
	};
	int index = 0;
	int someThingDisappeared = 0;
	int i,j;
	for(i = 0;i < size;i++){
		retval.ptr.items[index] = toBeSorted.ptr.items[i];
		for(j = i + 1;j < size && _isSameMonomial(toBeSorted.ptr.items[i],toBeSorted.ptr.items[j]);j++){
			addK(retval.ptr.items[index].coefficient,
				retval.ptr.items[index].coefficient, toBeSorted.ptr.items[j].coefficient);
			free(toBeSorted.ptr.items[j].degrees);
			freeK(toBeSorted.ptr.items[j].coefficient);
		}
		if(!cmpK(retval.ptr.items[index].coefficient , K_0)){
			free(retval.ptr.items[index].degrees);
			index--;
			someThingDisappeared = 1;
		}else{
			int k;
			for(k = 0;k < retval.ptr.items[index].size;k++){
				if(retval.ptr.items[index].degrees[k]){
					retval.ptr.items[index].degrees[k] = 1;
				}
			}
		}
		i = j - 1;
		index++;
	}
	free(toBeSorted.ptr.items);
	if(index == 0 && someThingDisappeared){
		retval = K2Poly(K_0,order);
	}else if(polySize(retval) != index){
		retval.ptr.items = realloc(retval.ptr.items,sizeof(*(retval.ptr.items))*(index+1));
		setPolySize(retval,index);
	}
	return retval;
}
Poly mkPolyArray(mut Poly *array,size_t size){
	Poly retval;
	retval.size = 0;
	retval.ptr.polies = array;
	setPolySize(retval,size);
	setPolyType(retval,ARRAY);
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
		newPtr =  realloc(newPtr,sizeof(*newPtr)*size);
	}
	setPolySize(poly,size);
	poly.ptr.polies = newPtr;
	return poly;
}
Poly removeUnnecessaryPolies(Poly grobner){
	int somethingHappened;
	do{
		somethingHappened = 0;
		size_t size = polySize(grobner);
		size_t i;
		Poly *ptr = unwrapPolyArray(grobner);
		ptr = realloc(ptr,sizeof(Poly)*size * 2);
		for(i = 0;i < size;i++){
			Poly _divisors = mkPolyArray(&ptr[i+1],size - 1);
			Poly divisors = removeNullPolyFromArray(polyDup(_divisors));
			Poly _result = polyDiv(ptr[i],divisors); 
			polyFree(divisors);
			Poly *result = unwrapPolyArray(_result);
			size_t r_index = polySize(_result) - 1;
			setPolySize(_result,r_index);
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
			}
			polyFree(ptr[i]);
			ptr[i] = nullPoly; 
			polyFree(_result);
			ptr[i + size] = r;
		}
		size *= 2;
		grobner.ptr.polies = ptr;
		setPolySize(grobner,size);
		grobner = removeNullPolyFromArray(grobner);
	}while(somethingHappened);
	return grobner;
}

/*Returns 0 if array is grobner basis*/
/*Returns reminder of S(g_i,g_j) divided by array*/
Poly isThisGrobnerBasis(Poly array){
	Poly *ptr = unwrapPolyArray(array);
	size_t s = polySize(array);
	size_t i,j;
	Poly reminder;
	for(i = 0;i < s;i++){
		for(j = i + 1;j < s;j++){
			if(_isCoprime(__polyIn(ptr[i]),__polyIn(ptr[j]))){
				continue;
			}
			Poly s_i_j = polyS(ptr[i],ptr[j]);
			reminder = polySim(s_i_j,array);polyFree(s_i_j);
			if(isZeroPoly(reminder)){
				polyFree(reminder);
			}else{
				goto ret;
			}
		}
	}
	reminder = K2Poly(K_0,polyType(ptr[0]));
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
			polyNice(ptr[i]);
		}
	}
	grobner = removeNullPolyFromArray(grobner);
	grobner = removeUnnecessaryPolies(grobner);
	
	ptr = unwrapPolyArray(grobner);
	size = polySize(grobner);
	for(i = 0;i < size;i++){
		Poly p = ptr[i];
		polyNice(p);
	}
	return grobner;
}

