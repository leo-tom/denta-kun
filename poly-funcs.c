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
	.ptr.terms = NULL
};

Poly zeroPoly = {
	.size = 1,
};

/*
S.<x,y,z> = PolynomialRing(QQ, 3, order='deglex')
J = Ideal([x^3 - 3*x^2 - y + 1, -x^2 + y^2 - 1])
J.groebner_basis()

*/

extern int _isSameMonomial(unmut Term v1,unmut Term v2);
extern int _polycmp_LEX(const Term *v1,const Term *v2);
extern int _polycmp_RLEX(const Term *v1,const Term *v2);
extern int _polycmp_PLEX(const Term *v1,const Term *v2);
extern int _polycmp_PRLEX(const Term *v1,const Term *v2);
extern N getDegreeOfATerm(const Term term);

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
	}else if(order == PRLEX){
		poly->size |= (int64_t)MONOMIAL_ORDER_IN_BIN__PRLEX << 60;
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
	retval.ptr.terms = malloc(sizeof(Term));
	setTermSize(retval.ptr.terms[0],0);
	termDegreeAllocator(retval.ptr.terms[0]);
	copyK(retval.ptr.terms[0].coefficient, k);
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
			termFree(v.ptr.terms[i]);
		}
	}
	if(size > 0){
		free(v.ptr.terms);
	}
}
int isNullPoly(unmut Poly poly){
	return poly.size == nullPoly.size
			&& poly.ptr.terms == nullPoly.ptr.terms;
}
int isZeroPoly(unmut Poly poly){
	return polySize(poly) == 1 && !cmpK(poly.ptr.terms[0].coefficient,K_0);
}
N polyDegrees(unmut Poly p){
	N max = 0;
	size_t i;
	for(i = 0;i < polySize(p);i++){
		N val = getDegreeOfATerm(p.ptr.terms[i]);
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
		Poly *to = malloc(sizeof(Poly)*polySize(poly));
		Poly *from = poly.ptr.polies;
		retval.ptr.polies = to;
		size_t i;
		for(i = 0;i < polySize(poly);i++){
			to[i] = polyDup(from[i]);
		}
	}else{
		retval.ptr.terms = malloc(sizeof(Term)*polySize(poly));
		size_t i;
		for(i = 0;i < polySize(poly);i++){
			retval.ptr.terms[i] = dupTerm(poly.ptr.terms[i]);
		}
	}
	return retval;
}

double poly2Double(unmut Poly poly){
	if(poly.size != 1 || termSize(poly.ptr.terms[0]) > 0){
		fprintf(stderr,"Trying to call poly2Double to : ");polyPrint(poly,K2str,stderr);fprintf(stderr,"\n");
		DIE;
	}
	return K2double(poly.ptr.terms[0].coefficient);
}

void polyPrint(unmut Poly poly,char*(*printer)(K),FILE *fp){
	size_t size = polySize(poly);
	if(isZeroPoly(poly)){
		fprintf(fp,"0");
		return;
	}else if(isNullPoly(poly)){
		fprintf(fp,"null");
		return;
	}else if(polyType(poly) == ARRAY){
		Poly *head = poly.ptr.polies;
		fprintf(fp,"(");
		while(size--){
			polyPrint(*head,printer,fp);
			if(size != 0){
				fprintf(fp,", ");
			}
			if(polyType(*head) == ARRAY){
				fprintf(fp,"\n");
			}
			head++;
		}
		fprintf(fp,")");
		return;
	}
	size_t index = 0;
	while(size-- > 0){
		unmut Term term = poly.ptr.terms[index++];
		if(!cmpK(term.coefficient,K_0)){
			fprintf(stderr,"This should not be happening\n");
			DIE;
		}else if(termSize(term) > 0 && (!cmpK(term.coefficient,K_1) 
				||!cmpK(term.coefficient,K_N1)
					) ){
			if(index != 1 && !cmpK(term.coefficient,K_1)){
				fprintf(fp,"+ ");
			}else if(!cmpK(term.coefficient,K_N1)){
				#if BOOLEAN
				if(index != 1) {
					fprintf(fp,"+ ");
				}
				#else
				fprintf(fp,"- ");
				#endif
			}else{
				DIE;
			}
		}else{
			//This is constant term
			char *buff;
			if(index == 1){
				fprintf(fp,"%s ",buff = printer(term.coefficient));
			}else if(cmpK(term.coefficient,K_0) < 0){
				K tmp;
				initK(tmp);
				mulK(tmp,K_N1,term.coefficient);
				fprintf(fp," - %s ",buff = printer(tmp));
			}else{
				fprintf(fp," + %s ",buff = printer(term.coefficient));
			}
			free(buff);
		}
		size_t i;
		for(i = 0; i < termSize(term);i++){
			if(termDegree(term,i)){
				fprintf(fp,"x_{%ld}",i+SUBSHIFT);
				if(termDegree(term,i) == 1){
					fprintf(fp," ");
				}else{
					fprintf(fp,"^{%ld} ",termDegree(term,i));
				}
			}
		}
	}
}
Poly term2Poly(mut Term term){
	Poly retval = {.size = 0};
	retval.ptr.terms = malloc(sizeof(Term));
	setPolySize(retval,1);
	setPolyType(retval,LEX);
	retval.ptr.terms[0].deg = term.deg;
	retval.ptr.terms[0].sizu = term.sizu;
	copyK(retval.ptr.terms[0].coefficient, term.coefficient);
	return retval;
}
Term __polyIn(unmut Poly poly){
	if(polySize(poly) <= 0){
		fprintf(stderr,"This poly has no items\n");
		DIE;
	}
	return poly.ptr.terms[0];
}
Term _polyIn(unmut Poly poly){
	return dupTerm(__polyIn(poly));
}
Poly polyIn(unmut Poly poly){
	Poly retval = term2Poly(_polyIn(poly));
	MonomialOrder order = polyType(poly);
	setPolyType(retval,order);
	return retval;
}
int cmpTerm(MonomialOrder order,Term v1,Term v2){
	int (*cmp)(const Term *,const Term *) = NULL;
	switch(order){
		case LEX : {cmp = _polycmp_LEX;break;}
		case RLEX : {cmp = _polycmp_RLEX;break;}
		case PLEX : {cmp = _polycmp_PLEX;break;}
		case PRLEX : {cmp = _polycmp_PRLEX;break;}
		default   : { DIE;}
	}
	return cmp(&v1,&v2);
}
int polyCmp(unmut Poly v1,unmut Poly v2){
	if(polyType(v1) != polyType(v2)){
		fprintf(stderr,"You cannot compare polies whose polynomial order are different.\n");
		DIE;
	}
	MonomialOrder order = polyType(v1);
	if(order == ARRAY){
		DIE;
	}
	size_t size = polySize(v1) > polySize(v2) ? polySize(v2) : polySize(v1);
	size_t i;
	for(i = 0;i < size;i++){
		int val = cmpTerm(order,v1.ptr.terms[i],v2.ptr.terms[i]);
		if( val != 0 ){
			return val;
		}
	}
	return 0;
}
Poly polyAdd(unmut Poly v1,unmut Poly v2){
	if(polyType(v1) != polyType(v2)){
		fprintf(stderr,"Trying to add Poly sorted by different monomial order\n");
		DIE;
	}
	if(isNullPoly(v1) || isNullPoly(v2)){
		return nullPoly;
	}
	if(isZeroPoly(v1)){
		return polyDup(v2);
	}else if(isZeroPoly(v2)){
		return polyDup(v1);
	}
	int (*cmp)(const Term *v1,const Term *v2) = NULL;
	switch(polyType(v1)){
		case LEX : {cmp = _polycmp_LEX;break;}
		case RLEX : {cmp = _polycmp_RLEX;break;}
		case PLEX : {cmp = _polycmp_PLEX;break;}
		case PRLEX : {cmp = _polycmp_PRLEX;break;}
		default   : { DIE;}
	}
	int i,j,index;
	i = j = index = 0;
	Poly retval = {
		.ptr.terms = malloc(sizeof(Term) * (polySize(v1) + polySize(v2)))
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
			cmpVal = cmp(&v1.ptr.terms[i],&v2.ptr.terms[j]);
		}
		if(cmpVal > 0){
			retval.ptr.terms[index++] = dupTerm(v1.ptr.terms[i++]);
		}else if(cmpVal < 0){
			retval.ptr.terms[index++] = dupTerm(v2.ptr.terms[j++]);
		}else{
			retval.ptr.terms[index] = dupTerm(v1.ptr.terms[i++]);
			addK(retval.ptr.terms[index].coefficient
					,retval.ptr.terms[index].coefficient
					,v2.ptr.terms[j++].coefficient);
			if(!cmpK(retval.ptr.terms[index].coefficient,K_0)){
				someThingDisappeared = 1;
				termFree(retval.ptr.terms[index]);
			}else{
				index++;
			}
		}
	}
	if(index == 0 && someThingDisappeared){
		free(retval.ptr.terms);
		retval = polyDup(zeroPoly);
	}else if(index < polySize(retval)){
		retval.ptr.terms = realloc(retval.ptr.terms,sizeof(Term)*index);
		setPolySize(retval,index);
	}
	return retval;
}
Poly polySub(unmut Poly v1,unmut Poly v2){
	size_t i;
	for(i = 0;i < polySize(v2);i++){
		mulK(v2.ptr.terms[i].coefficient,v2.ptr.terms[i].coefficient,K_N1);
	}
	Poly retval = polyAdd(v1,v2);
	for(i = 0;i < polySize(v2);i++){
		mulK(v2.ptr.terms[i].coefficient,v2.ptr.terms[i].coefficient,K_N1);
	}
	return retval;
}
Poly polyMul(unmut Poly v1,unmut Poly v2){
	if(polyType(v1) != polyType(v2)){
		fprintf(stderr,"Trying to multiply Poly sorted by different monomial order\n");
		DIE;
	}
	if(isNullPoly(v1) || isNullPoly(v2)){
		return nullPoly;
	}
	if(isZeroPoly(v1) || isZeroPoly(v2)){
		return polyDup(zeroPoly);
	}
	
	int i,j,k,index;
	if(polySize(v1) == 0 || polySize(v2) == 0){
		fprintf(stderr,"This should not be happening\n");
		DIE;
	}
	Poly retval = {
		.size = 0,
	};
	retval.ptr.terms = malloc(sizeof(Term)*polySize(v1)*polySize(v2)); 
	index = 0;
	for(i = 0;i < polySize(v1);i++){
		for(j = 0;j < polySize(v2);j++){
			Term bigger,smaller;
			if(termSize(v1.ptr.terms[i]) > termSize(v2.ptr.terms[j])){
				bigger = v1.ptr.terms[i];
				smaller = v2.ptr.terms[j];
			}else{
				bigger = v2.ptr.terms[j];
				smaller = v1.ptr.terms[i];
			}
			retval.ptr.terms[index] = dupTerm(bigger);
			for(k = 0;k < termSize(smaller) ;k++){
				#if BOOLEAN
					if(termDegree(smaller,k)){
						setTermDegree(retval.ptr.terms[index],k,1);
					}
				#else
					N deg = termDegree(bigger,k) + termDegree(smaller,k);
					setTermDegree(retval.ptr.terms[index],k,deg);
				#endif
			}
			mulK(retval.ptr.terms[index].coefficient,retval.ptr.terms[index].coefficient, smaller.coefficient);
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

int _isDiviable(unmut Term dividend,unmut Term divisor){
	if(termSize(dividend) <  termSize(divisor)){
		return 0;
	}
	size_t i;
	for(i = 0;i < termSize(divisor);i++){
		#if BOOLEAN
		if(termDegree(dividend,i) == 0 && termDegree(divisor,i) == 1){
			return 0;
		}
		#else
		if(termDegree(dividend,i) < termDegree(divisor,i)){
			return 0;
		}
		#endif
	}
	return 1;
}

/*This function returns array of Poly whose length is (size + 1).*/
/*divisor must be array of Poly whose length is 'size'*/
Poly polyDiv(unmut Poly dividend,unmut Poly divisors){
	int i,j,k;
	MonomialOrder order = polyType(dividend);
	size_t size;
	Poly h = polyDup(dividend);
	Poly *divisor;
	if(polyType(divisors) == ARRAY){
		divisor = unwrapPolyArray(divisors);
		size = polySize(divisors);
	}else{
		divisor = &divisors;
		size = 1;
	}
	Term *in_g = malloc(sizeof(Term) * size);
	int error = 1;
	for(i = 0;i < size;i++){
		in_g[i] = __polyIn(divisor[i]);
		if(error && !isZeroPoly(divisor[i])){
			error = 0;
		}
	}
	Poly *retval = malloc(sizeof(Poly) * (size + 1));
	for(i = 0;i < size;i++){
		retval[i] = polyDup(zeroPoly);
		setPolyType(retval[i],order);
	}
	if(error){
		goto ret;
	}
	while(!isZeroPoly(h)){
		for(i = 0;i < polySize(h);i++){
			for(j = 0;j < size;j++){
				if(_isDiviable(h.ptr.terms[i],in_g[j])){
					goto found;
				}
			}
		}
		break;
		found : {
			Term u = h.ptr.terms[i];
			Term g = in_g[j];
			Term w;
			size_t _size_tmp = termSize(u);
			setTermSize(w,_size_tmp);
			termDegreeAllocator(w);
			for(k = 0;k < termSize(g);k++){
				N newDeg = termDegree(u,k) - termDegree(g,k);
				setTermDegree(w,k,newDeg);
			}
			for(;k < termSize(u);k++){
				N newDeg = termDegree(u,k);
				setTermDegree(w,k,newDeg);
			}
			for(;k > 0;k--){
				if(termDegree(w,(k-1))){
					break;
				}
			}
			if(termSize(w) != k){
				setTermSize(w,k);
				Term newTerm = dupTerm(w);
				termFree(w);
				w = newTerm;
			}
			initK(w.coefficient);
			divK(w.coefficient,u.coefficient , g.coefficient);
			mulK(w.coefficient,w.coefficient , K_N1);
			Poly tmp = term2Poly(w);
			setPolyType(tmp,order);
			Poly temp = polyMul(tmp,divisor[j]);
			Poly tempo = polyAdd(h,temp);
			polyFree(h);
			polyFree(temp);
			h = tempo;
			Poly newVal = polyAdd(retval[j],tmp);
			polyFree(tmp);
			polyFree(retval[j]);
			retval[j] = newVal;
		}
	}
	ret : 
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
					
N __max(Term x,Term y,size_t index){
	if(index >= termSize(x) && index >= termSize(y)){
		DIE;
	}
	if(index >= termSize(x)){
		return termDegree(y,index);
	}else if(index >= termSize(y)){
		return termDegree(x,index);
	}else if(termDegree(x,index) > termDegree(y,index)){
		return termDegree(x,index);
	}else{
		return termDegree(y,index);
	}
}

Poly polyS(unmut Poly f,unmut Poly g){
	MonomialOrder order = polyType(f);
	if(order != polyType(g)){
		fprintf(stderr,"S polynomial of different polynomials cannnot be computed\n");
		DIE;
	}
	Term in_f = __polyIn(f);
	Term in_g = __polyIn(g);
	Term w_f,w_g;
	size_t sizeofBiggerTerm = (termSize(in_f) < termSize(in_g)) ? termSize(in_g) : termSize(in_f);
	setTermSize(w_f,sizeofBiggerTerm);
	setTermSize(w_g,sizeofBiggerTerm);
	termDegreeAllocator(w_f);
	termDegreeAllocator(w_g);
	initK(w_f.coefficient);
	initK(w_g.coefficient);
	size_t i;
	for(i = 0;i < termSize(in_f);i++){
		N newDeg = __max(in_f,in_g,i) - termDegree(in_f,i);
		setTermDegree(w_f,i,newDeg);
	}
	for(;i < termSize(w_f);i++){
		setTermDegree(w_f,i,__max(in_f,in_g,i));
	}
	for(;i > 0;i--){
		if(termDegree(w_f,(i-1))){
			break;
		}
	}
	if(termSize(w_f) != i){
		setTermSize(w_f,i);
		Term tmp = dupTerm(w_f);
		termFree(w_f);
		w_f = tmp;
	}
	for(i = 0;i < termSize(in_g);i++){
		N newDeg = __max(in_f,in_g,i) - termDegree(in_g,i);
		setTermDegree(w_g,i,newDeg);
	}
	for(;i < termSize(w_g);i++){
		setTermDegree(w_g,i,__max(in_f,in_g,i));
	}
	for(;i > 0;i--){
		if(termDegree(w_g,(i-1))){
			break;
		}
	}
	if(termSize(w_g) != i){
		setTermSize(w_g,i);
		Term tmp = dupTerm(w_g);
		termFree(w_g);
		w_g = tmp;
	}
	divK(w_f.coefficient,K_1,in_f.coefficient);
	divK(w_g.coefficient,K_1,in_g.coefficient);
	Poly w_f_p = term2Poly(w_f);
	Poly w_g_p = term2Poly(w_g);
	setPolyType(w_f_p,order);
	setPolyType(w_g_p,order);
	Poly tmp_f = polyMul(w_f_p,f); polyFree(w_f_p);
	Poly tmp_g = polyMul(w_g_p,g); polyFree(w_g_p);
	Poly retval = polySub(tmp_f,tmp_g); polyFree(tmp_f);polyFree(tmp_g);
	return retval;
}

Term dupTerm(unmut Term term){
	Term retval;
	size_t size = termSize(term);
	setTermSize(retval,size);
	copyK(retval.coefficient,term.coefficient);
	if(size){
		termDegreeAllocator(retval);
		#if BOOLEAN
		if(size <= sizeof(N)*8){
			retval.deg.val = term.deg.val;
		}else{
			memcpy(retval.deg.ptr,term.deg.ptr,sizeof(N)*((size - 1)/(sizeof(N)*8) + 1));
		}
		#else
		memcpy(retval.deg.ptr,term.deg.ptr,sizeof(N)*size);
		#endif
	}else{
		termDegreeAllocator(retval);
	}
	return retval; 
}

N getDegreeOfATerm(const Term term){
	N retval = 0;
	size_t i;
	#if BOOLEAN
	if(termSize(term) == 0){
		return 0;
	}else if(termSize(term) <= (sizeof(N)*8)){
		retval = popcount(term.deg.val);
	}else{
		for(i = 0;i < ((termSize(term)-1)/(sizeof(N)*8) + 1);i++){
			retval += popcount(term.deg.ptr[i]);
		}
	}
	#else
	for(i = 0;i < termSize(term);i++){
		retval += termDegree(term,i);
	}
	#endif
	return retval;
}

// v1 > v2 => +1
int _polycmp_LEX(const Term *_v1,const Term *_v2){
	const Term v1 = *_v1;
	const Term v2 = *_v2;
	N degree_v1 = getDegreeOfATerm(v1);
	N degree_v2 = getDegreeOfATerm(v2);
	if(degree_v1 != degree_v2){
		return (degree_v1 < degree_v2) ? -1 : +1;
	}
	return _polycmp_PLEX(_v1,_v2); 
}

int _polycmp_RLEX(const Term *_v1,const Term *_v2){
	const Term v1 = *_v1;
	const Term v2 = *_v2;
	N degree_v1 = getDegreeOfATerm(v1);
	N degree_v2 = getDegreeOfATerm(v2);
	if(degree_v1 != degree_v2){
		return (degree_v1 < degree_v2) ? -1 : +1;
	}
	return _polycmp_PRLEX(_v1,_v2);
}
int _polycmp_PLEX(const Term *_v1,const Term *_v2){
	const Term v1 = *_v1;
	const Term v2 = *_v2;
	size_t i;
	for(i = 0;i < termSize(v1) && i < termSize(v2);i++){
		if(termDegree(v2,i) != termDegree(v1,i)){
			return ((termDegree(v2,i) > termDegree(v1,i))) ? -1 : +1;
		}
	}
	if(termSize(v1) == termSize(v2)){
		return 0;
	}else if(termSize(v2) > termSize(v1)){
		return -1;
	}else{
		return +1;
	}
}
int _polycmp_PRLEX(const Term *_v1,const Term *_v2){
	const Term v1 = *_v1;
	const Term v2 = *_v2;
	int64_t i;
	if(termSize(v1) > termSize(v2)){
		return -1;
	}else if(termSize(v1) < termSize(v2)){
		return 1;
	}
	for(i = termSize(v1) - 1;i >= 0;i--){
		if(termDegree(v2,i) - termDegree(v1,i) != 0){
			return ((termDegree(v2,i) < termDegree(v1,i))) ? -1 : +1;
		}
	}
	return 0;
}
// v1 > v2 => -1
int polyCMP_LEX(const Term *v1,const Term *v2){
	return -1*_polycmp_LEX(v1,v2);
}
int polyCMP_RLEX(const Term *v1,const Term *v2){
	return -1*_polycmp_RLEX(v1,v2);
}
int polyCMP_PLEX(const Term *v1,const Term *v2){
	return -1*_polycmp_PLEX(v1,v2);
}
int polyCMP_PRLEX(const Term *v1,const Term *v2){
	return -1*_polycmp_PRLEX(v1,v2);
}

int _isSameMonomial(unmut Term v1,unmut Term v2){
	if(termSize(v1) != termSize(v2)){
		return 0;
	}
	size_t i;
	for(i=0;i < termSize(v1);i++){
		if(termDegree(v1,i) != termDegree(v2,i)){
			return 0;
		}
	}
	return 1;
}

void polyNice(unmut Poly p){
	#if BOOLEAN
	return;
	#else
	if(polySize(p) == 0 || !cmpK(p.ptr.terms[0].coefficient,K_0)
			|| !cmpK(p.ptr.terms[0].coefficient,K_1)){
		return;
	}
	size_t j;
	K gyaku;
	initK(gyaku);
	divK(gyaku,K_1,p.ptr.terms[0].coefficient);
	for(j = 0;j < polySize(p);j++){
		mulK(p.ptr.terms[j].coefficient,
		gyaku,p.ptr.terms[j].coefficient);
	}
	freeK(gyaku);
	#endif
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
		case LEX : {qsort(toBeSorted.ptr.terms,size,sizeof(Term)
					,(int (*)(const void *,const void *))polyCMP_LEX); setPolyType(toBeSorted,LEX); break;}
		case RLEX : {qsort(toBeSorted.ptr.terms,size,sizeof(Term)
					,(int (*)(const void *,const void *))polyCMP_RLEX); setPolyType(toBeSorted,RLEX); break;}
		case PLEX : {qsort(toBeSorted.ptr.terms,size,sizeof(Term)
					,(int (*)(const void *,const void *))polyCMP_PLEX); setPolyType(toBeSorted,PLEX); break;}
		case PRLEX : {qsort(toBeSorted.ptr.terms,size,sizeof(Term)
					,(int (*)(const void *,const void *))polyCMP_PRLEX); setPolyType(toBeSorted,PRLEX); break;}
		default   : { DIE;}
	}
	Poly retval = {
		.size = toBeSorted.size, /*Copy entire toBeSorted.size. That is, both type of monomial order and size of terms*/
		.ptr.terms = malloc(sizeof(Term) * polySize(toBeSorted))
	};
	size_t index = 0;
	int someThingDisappeared = 0;
	size_t i,j;
	for(i = 0;i < size;i++){
		retval.ptr.terms[index] = toBeSorted.ptr.terms[i];
		for(j = i + 1;j < size && _isSameMonomial(toBeSorted.ptr.terms[i],toBeSorted.ptr.terms[j]);j++){
			addK(retval.ptr.terms[index].coefficient,
				retval.ptr.terms[index].coefficient, toBeSorted.ptr.terms[j].coefficient);
			termFree(toBeSorted.ptr.terms[j]);
		}
		if(!cmpK(retval.ptr.terms[index].coefficient , K_0)){
			termFree(retval.ptr.terms[index]);
			someThingDisappeared = 1;
		}else{
			index++;
		}
		i = j - 1;
	}
	free(toBeSorted.ptr.terms);
	if(index == 0 && someThingDisappeared){
		free(retval.ptr.terms);
		retval = polyDup(zeroPoly);
		setPolyType(retval,order);
	}else if(polySize(retval) != index){
		retval.ptr.terms = realloc(retval.ptr.terms,sizeof(Term)*index);
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
int _isCoprime(Term v1,Term v2){
	size_t s = termSize(v1) > termSize(v2) ? termSize(v2) : termSize(v1);
	size_t i;
	for(i = 0;i < s;i++){
		if(termDegree(v1,i) && termDegree(v2,i)){
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
	if(polySize(grobner) <= 1){
		return grobner;
	}
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
			Poly spoly = polyS(ptr[i],ptr[j]);
			reminder = polySim(spoly,array);polyFree(spoly);
			if(isZeroPoly(reminder)){
				polyFree(reminder);
			}else{
				goto ret;
			}
			spoly = polyS(ptr[i],ptr[j]);
			reminder = polySim(spoly,array);polyFree(spoly);
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
	size_t i,index = 0;
	size_t size = polySize(grobner);
	int64_t currentSize = size;
	Poly *ptr = unwrapPolyArray(grobner);
	Poly *newArray = malloc(sizeof(Poly) * size);
	while(currentSize > 0){
		for(i = 0;i < size;i++){
			Poly g = polyDup(ptr[i]);
			if(isNullPoly(g)){
				continue;
			}
			size_t j;
			for(j = 0; j < size;j++){
				if(isNullPoly(ptr[j])){
					continue;
				}
				if(_isDiviable(__polyIn(ptr[j]),__polyIn(g))){
					polyFree(ptr[j]);
					ptr[j] = nullPoly;
					currentSize--;
					break;
				}
			}
			int addG = 1;
			for(j = 0;j < size;j++){
				if(isNullPoly(ptr[j])){
					continue;
				}
				if(_isDiviable(__polyIn(g),__polyIn(ptr[j]))){
					addG = 0;
					break;
				}
			}
			if(addG){
				newArray[index++] = g;
			}
		}
	}
	polyFree(grobner);
	grobner = mkPolyArray(newArray,index);
	grobner = removeUnnecessaryPolies(grobner);
	
	ptr = unwrapPolyArray(grobner);
	size = polySize(grobner);
	for(i = 0;i < size;i++){
		Poly p = ptr[i];
		polyNice(p);
	}
	return grobner;
}

