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

extern N getDegreeOfATerm(const Term term);

void _setPolySize(Poly *poly,size_t size){
	poly->size &= ((int64_t)0xf) << 60;
	poly->size |= (0xfffffffffffffff & size);
}
void _setPolyType(Poly *poly,PolyType type){
	poly->size &= 0xfffffffffffffff;
	switch(type){
		case POLY : {
			poly->size |= (int64_t)POLYTYPE_POLY << 60 ;
		}break;
		case ARRAY : {
			poly->size |= (int64_t)POLYTYPE_ARRAY << 60 ;
		}
	}
}

Poly K2Poly(unmut K k){
	Poly retval;
	setPolySize(retval,1);
	setPolyType(retval,POLY);
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
		Term *ptr = v.ptr.terms;
		for(i=0;i < size;i++){
			termFree(*ptr);
			ptr++;
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
	#if DEBUG
	if(polySize(poly) == 0){
		return nullPoly;
	}
	#endif
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
	if(polyType(poly) == ARRAY || polySize(poly) != 1 || termSize(poly.ptr.terms[0]) > 0){
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
		int64_t i;
		for(i = 0; i < termSize(term);i++){
			if(termDegree(term,i)){
				fprintf(fp,"x_{%ld}",i - SUBSHIFT);
				if(termDegree(term,i) == 1){
					fprintf(fp," ");
				}else{
					fprintf(fp,"^{%ld} ",termDegree(term,i));
				}
			}
		}
	}
	fflush(fp);
}
void polyPrintBori(unmut Poly poly,FILE *fp){
	size_t size = polySize(poly);
	if(isZeroPoly(poly)){
		fprintf(fp,"0");
		return;
	}else if(isNullPoly(poly)){
		fprintf(fp,"null");
		return;
	}else if(polyType(poly) == ARRAY){
		Poly *head = poly.ptr.polies;
		fprintf(fp,"[");
		while(size--){
			polyPrintBori(*head,fp);
			if(size != 0){
				fprintf(fp,", ");
			}
			if(polyType(*head) == ARRAY){
				fprintf(fp,"\n");
			}
			head++;
		}
		fprintf(fp,"]");
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
				if(index != 1) {
					fprintf(fp,"+ ");
				}
			}else{
				DIE;
			}
		}else{
			if(index == 1){
				if(!cmpK(term.coefficient,K_1)){
					fprintf(fp," 1 ");
				}else{
					fprintf(fp," 0 ");
				}
			}else if(!cmpK(term.coefficient,K_1)){
				fprintf(fp," + 1 ");
			}
		}
		int64_t i;
		for(i = 0; i < termSize(term);i++){
			if(termDegree(term,i)){
				int64_t index = i - SUBSHIFT;
				if(index < 0){
					fprintf(fp,"x%ld",1000 - index);
				}else{
					fprintf(fp,"x%ld",index);
				}
				break;
			}
		}
		for(i = i+1; i < termSize(term);i++){
			if(termDegree(term,i)){
				int64_t index = i - SUBSHIFT;
				if(index < 0){
					fprintf(fp,"*x%ld ",1000 - index);
				}else{
					fprintf(fp,"*x%ld ",index);
				}
			}
		}
	}
	fflush(fp);
}
Poly term2Poly(mut Term term){
	Poly retval = {.size = 0};
	retval.ptr.terms = malloc(sizeof(Term));
	setPolySize(retval,1);
	setPolyType(retval,POLY);
	retval.ptr.terms[0] = term;
	return retval;
}
Term __polyIn(unmut Poly poly){
	#if DEBUG
	if(polySize(poly) <= 0){
		fprintf(stderr,"This poly has no items\n");
		DIE;
	}
	#endif
	return poly.ptr.terms[0];
}
Term _polyIn(unmut Poly poly){
	return dupTerm(__polyIn(poly));
}
Poly polyIn(unmut Poly poly){
	Poly retval = term2Poly(_polyIn(poly));
	return retval;
}
int cmpTerm(const Term v1,const Term v2){
	return _cmpTerm(&v1,&v2);
}
int polyCmp(unmut Poly v1,unmut Poly v2){
	if(polyType(v1) == ARRAY || polyType(v2) == ARRAY){
		fprintf(stderr,"You cannot compare arrays.\n");
		DIE;
	}
	size_t size = polySize(v1) > polySize(v2) ? polySize(v2) : polySize(v1);
	size_t i;
	for(i = 0;i < size;i++){
		int val = cmpTerm(v1.ptr.terms[i],v2.ptr.terms[i]);
		if( val != 0 ){
			return val;
		}else if(cmpK(v1.ptr.terms[i].coefficient,v2.ptr.terms[i].coefficient)){
			return cmpK(v1.ptr.terms[i].coefficient,v2.ptr.terms[i].coefficient);
		}
	}
	return 0;
}
Poly _polyAdd(mut Poly v1,mut Poly v2){
	#if DEBUG
	if(polyType(v1) == ARRAY || polyType(v2) == ARRAY){
		fprintf(stderr,"You cant add arrays\n");
		DIE;
	}
	if(isNullPoly(v1) || isNullPoly(v2)){
		polyFree(v1);
		polyFree(v2);
		return nullPoly;
	}
	#endif
	if(isZeroPoly(v1)){
		polyFree(v1);
		return v2;
	}else if(isZeroPoly(v2)){
		polyFree(v2);
		return v1;
	}

	int i,j,index;
	i = j = index = 0;
	Poly retval = {
		.ptr.terms = malloc(sizeof(Term) * (polySize(v1) + polySize(v2)))
	};
	setPolySize(retval,(polySize(v1) + polySize(v2)));
	setPolyType(retval,POLY);
	int someThingDisappeared = 0;
	while(i < polySize(v1) || j < polySize(v2)){
		int cmpVal;
		if(i >= polySize(v1)){
			cmpVal = -1;
		}else if(j >= polySize(v2)){
			cmpVal = +1;
		}else{
			cmpVal = cmpTerm(v1.ptr.terms[i],v2.ptr.terms[j]);
		}
		if(cmpVal > 0){
			retval.ptr.terms[index++] = v1.ptr.terms[i++];
		}else if(cmpVal < 0){
			retval.ptr.terms[index++] = v2.ptr.terms[j++];
		}else{
			#if BOOLEAN
				termFree(v1.ptr.terms[i]);
				termFree(v2.ptr.terms[j]);
				i++;j++;
				someThingDisappeared = 1;
			#else
			retval.ptr.terms[index] = v1.ptr.terms[i++];
			addK(retval.ptr.terms[index].coefficient
					,retval.ptr.terms[index].coefficient
					,v2.ptr.terms[j].coefficient);
			termFree(v2.ptr.terms[j]);
			j++;
			if(!cmpK(retval.ptr.terms[index].coefficient,K_0)){
				someThingDisappeared = 1;
				termFree(retval.ptr.terms[index]);
			}else{
				index++;
			}
			#endif
		}
	}
	free(v1.ptr.terms);
	free(v2.ptr.terms);
	if(index == 0 && someThingDisappeared){
		free(retval.ptr.terms);
		retval = polyDup(zeroPoly);
	}else if(index < polySize(retval)){
		retval.ptr.terms = realloc(retval.ptr.terms,sizeof(Term)*index);
		setPolySize(retval,index);
	}
	return retval;
}

Poly polyAdd(unmut Poly v1,unmut Poly v2){
	return _polyAdd(polyDup(v1),polyDup(v2));
}

Poly polySub(unmut Poly v1,unmut Poly v2){
	#if !BOOLEAN
	size_t i;
	for(i = 0;i < polySize(v2);i++){
		mulK(v2.ptr.terms[i].coefficient,v2.ptr.terms[i].coefficient,K_N1);
	}
	#endif
	Poly retval = polyAdd(v1,v2);
	#if !BOOLEAN
	for(i = 0;i < polySize(v2);i++){
		mulK(v2.ptr.terms[i].coefficient,v2.ptr.terms[i].coefficient,K_N1);
	}
	#endif
	return retval;
}

Poly _polyMul(mut Poly v1,mut Poly v2){
	#if DEBUG
	if(polyType(v1) == ARRAY || polyType(v2) == ARRAY){
		fprintf(stderr,"You can't multiply arrays\n");
		DIE;
	}
	if(isNullPoly(v1) || isNullPoly(v2)){
		polyFree(v1);
		polyFree(v2);
		return nullPoly;
	}
	if(polySize(v1) == 0 || polySize(v2) == 0){
		fprintf(stderr,"This should not be happening\n");
		polyPrint(v1,K2str,stderr);
		polyPrint(v2,K2str,stderr);
		v1.ptr.terms[0] = v2.ptr.terms[1];
		DIE;
	}
	#endif
	if(isZeroPoly(v1) || isZeroPoly(v2)){
		polyFree(v1);
		polyFree(v2);
		return polyDup(zeroPoly);
	}
	if(!polyCmp(v1,onePoly)){
		polyFree(v1);
		return v2;
	}else if(!polyCmp(v2,onePoly)){
		polyFree(v2);
		return v1;
	}
	
	int64_t index;
	
	Poly retval = {
		.size = 0,
	};
	const size_t v1s = polySize(v1);
	const size_t v2s = polySize(v2);
	size_t retvalSize = v1s * v2s;
	retval.ptr.terms = malloc(sizeof(Term)*retvalSize); 
	index = 0;
	Term *v1t = v1.ptr.terms;
	Term *v1tt = v1t + v1s;
	Term *v2t = v2.ptr.terms;
	Term *v2tt = v2t + v2s;
	for(;v1t < v1tt;v1t++){
		for(v2t = v2.ptr.terms;v2t < v2tt;v2t++){
			Term *bigger;
			Term *smaller;
			if(termSize(*v1t) > termSize(*v2t)){
				bigger = v1t;
				smaller = v2t;
			}else{
				bigger = v2t;
				smaller = v1t;
			}
			retval.ptr.terms[index] = dupTerm(*bigger);
			const size_t smallerSize = termSize(*smaller);
			size_t k;
			for(k = 0;k < smallerSize ;k++){
				#if BOOLEAN
					if(termDegree(*smaller,k)){
						setTermDegree(retval.ptr.terms[index],k,1);
					}
				#else
					N deg = termDegree(*bigger,k) + termDegree(*smaller,k);
					setTermDegree(retval.ptr.terms[index],k,deg);
				#endif
			}
			#if !BOOLEAN
			mulK(retval.ptr.terms[index].coefficient,retval.ptr.terms[index].coefficient, smaller->coefficient);
			#endif
			index++;			
		}
		if(index > 100 && index > (retvalSize/4)){
			setPolySize(retval,index);
			setPolyType(retval,POLY);
			retval = _polyNice(retval);
			retval.ptr.terms = realloc(retval.ptr.terms,sizeof(Term)*retvalSize);
			index = polySize(retval);
		}
	}
	setPolySize(retval,index);
	setPolyType(retval,POLY);
	polyFree(v1);
	polyFree(v2);
	return _polySort(retval);
}

Poly polyMul(unmut Poly v1,unmut Poly v2){
	return _polyMul(polyDup(v1),polyDup(v2));
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
	for(i = 0;i < size;i++){
		if(isZeroPoly(divisor[i]) || isNullPoly(divisor[i])){
			fprintf(stderr,"Division by zero or NULL\n");
			DIE;
		}
		in_g[i] = __polyIn(divisor[i]);
	}
	Poly *retval = malloc(sizeof(Poly) * (size + 1));
	for(i = 0;i < size;i++){
		retval[i] = polyDup(zeroPoly);
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
				if(termSize(w) > sizeof(N)*8 && k <= sizeof(N)*8){
					N tmp = w.deg.ptr[0];
					free(w.deg.ptr);
					w.deg.val = tmp;
					setTermSize(w,k);
				}else{
					setTermSize(w,k);
					Term newTerm = dupTerm(w);
					termFree(w);
					w = newTerm;
				}
			}
			#if BOOLEAN
				copyK(w.coefficient,K_1);
			#else
				initK(w.coefficient);
				divK(w.coefficient,u.coefficient , g.coefficient);
				mulK(w.coefficient,w.coefficient , K_N1);
			#endif
			Poly tmp = term2Poly(w);
			h = _polyAdd(h,polyMul(tmp,divisor[j]));
			#if DEBUG >= 2
				fprintf(stderr,"h : ");
				polyPrint(h,K2str,stderr);
				fprintf(stderr,"\n");
			#endif
			retval[j] = _polyAdd(retval[j],tmp);
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
	#if DEBUG
	if(polyType(f) == ARRAY || polyType(g) == ARRAY){
		DIE;
	}
	#endif
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
		N newDeg = __max(in_f,in_g,i);
		setTermDegree(w_f,i,newDeg);
	}
	for(;i > 0;i--){
		if(termDegree(w_f,(i-1))){
			break;
		}
	}
	if(termSize(w_f) != i){
		if(termSize(w_f) > sizeof(N)*8 && i <= sizeof(N)*8){
			N tmp = w_f.deg.ptr[0];
			free(w_f.deg.ptr);
			w_f.deg.val = tmp;
			setTermSize(w_f,i);
		}else{
			setTermSize(w_f,i);
			Term tmp = dupTerm(w_f);
			termFree(w_f);
			w_f = tmp;
		}
	}
	for(i = 0;i < termSize(in_g);i++){
		N newDeg = __max(in_f,in_g,i) - termDegree(in_g,i);
		setTermDegree(w_g,i,newDeg);
	}
	for(;i < termSize(w_g);i++){
		N newDeg = __max(in_f,in_g,i);
		setTermDegree(w_g,i,newDeg);
	}
	for(;i > 0;i--){
		if(termDegree(w_g,(i-1))){
			break;
		}
	}
	if(termSize(w_g) != i){
		if(termSize(w_g) > sizeof(N)*8 && i <= sizeof(N)*8){
			N tmp = w_g.deg.ptr[0];
			free(w_g.deg.ptr);
			w_g.deg.val = tmp;
			setTermSize(w_g,i);
		}else{
			setTermSize(w_g,i);
			Term tmp = dupTerm(w_g);
			termFree(w_g);
			w_g = tmp;
		}
	}
	divK(w_f.coefficient,K_1,in_f.coefficient);
	divK(w_g.coefficient,K_1,in_g.coefficient);
	Poly w_f_p = term2Poly(w_f);
	Poly w_g_p = term2Poly(w_g);

	Poly tmp_f = polyMul(w_f_p,f); polyFree(w_f_p);
	Poly tmp_g = polyMul(w_g_p,g); polyFree(w_g_p);
	Poly retval = polySub(tmp_f,tmp_g); polyFree(tmp_f);polyFree(tmp_g);
	#if DEBUG >= 2
		fprintf(stderr,"f : ");
		polyPrint(f,K2str,stderr);
		fprintf(stderr,"\n");
		fprintf(stderr,"g : ");
		polyPrint(g,K2str,stderr);
		fprintf(stderr,"\n");
		fprintf(stderr,"S : ");
		polyPrint(retval,K2str,stderr);
		fprintf(stderr,"\n");
	#endif
	return retval;
}

Term dupTerm(unmut Term term){
	Term retval;
	const size_t size = termSize(term);
	setTermSize(retval,size);
	termDegreeAllocator(retval);
	copyK(retval.coefficient,term.coefficient);
	if(size){
		#if BOOLEAN
		if(size <= sizeof(N)*8){
			retval.deg.val = term.deg.val;
		}else{
			memcpy(retval.deg.ptr,term.deg.ptr,sizeof(N)*(size/(sizeof(N)*8) + 1));
		}
		#else
		memcpy(retval.deg.ptr,term.deg.ptr,sizeof(N)*size);
		#endif
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
int _cmpTerm_r(const Term *v1,const Term *v2){
	return -1*_cmpTerm(v1,v2);
}
int cmpTerm_r(const Term v1,const Term v2){
	return -1*cmpTerm(v1,v2);
}

/*Delete terms which can be disappired*/
Poly _polyNice(mut Poly p){
	#if !BOOLEAN
		DIE;
	#endif
	size_t i,j,size;
	Term zeroTerm = {
		.deg.ptr = NULL,
		.sizu = 0
	};
	copyK(zeroTerm.coefficient,K_0);
	size = 0;
	for(i = 0;i < polySize(p);i++){
		Term term = p.ptr.terms[i];
		if(!cmpTerm(term,zeroTerm)){
			continue;
		}else{
			size = i + 1;
		}
		int counter = 1;
		for(j = i + 1;j < polySize(p);j++){
			if(!cmpTerm(term,p.ptr.terms[j])){
				termFree(p.ptr.terms[j]);
				p.ptr.terms[j] = zeroTerm;
				counter++;
			}
		}
		if(counter % 2 == 0){
			termFree(p.ptr.terms[i]);
			for(j = i + 1;j < polySize(p);j++){
				if(p.ptr.terms[j].coefficient != K_0){
					p.ptr.terms[i] = p.ptr.terms[j];
					p.ptr.terms[j] = zeroTerm;
					i--;
					break;
				}
			}
			if(j == polySize(p)){
				// all remaining terms are zero.
				setPolySize(p,i);
				p.ptr.terms = realloc(p.ptr.terms,sizeof(Term)*i);
				return p;
			}
		}
	}
	if(size == 0){
		free(p.ptr.terms);
		return polyDup(zeroPoly);
	}
	setPolySize(p,size);
	p.ptr.terms = realloc(p.ptr.terms,sizeof(Term)*size);
	return p;
}

void polyNice(mut Poly p){
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

Poly _polySort(mut Poly poly){
	if(polyType(poly) == ARRAY){
		size_t size = polySize(poly);
		size_t i;
		Poly *ptr = unwrapPolyArray(poly);
		for(i = 0;i < size;i++){
			ptr[i] = _polySort(ptr[i]);
		}
		return poly;
	}
	size_t size = polySize(poly);
	qsort(poly.ptr.terms,size,sizeof(Term),(int (*)(const void *,const void *))_cmpTerm_r);
	size_t index = 0;
	int someThingDisappeared = 0;
	size_t i,j;
	for(i = 0;i < size;i++){
		poly.ptr.terms[index] = poly.ptr.terms[i];
		for(j = i + 1;j < size && !cmpTerm(poly.ptr.terms[index],poly.ptr.terms[j]);j++){
			addK(poly.ptr.terms[index].coefficient,
				poly.ptr.terms[index].coefficient, poly.ptr.terms[j].coefficient);
			termFree(poly.ptr.terms[j]);
		}
		if(!cmpK(poly.ptr.terms[index].coefficient , K_0)){
			termFree(poly.ptr.terms[index]);
			someThingDisappeared = 1;
		}else{
			index++;
		}
		i = j - 1;
	}
	if(index == 0 && someThingDisappeared){
		free(poly.ptr.terms);
		poly = polyDup(zeroPoly);
	}else if(polySize(poly) != index){
		poly.ptr.terms = realloc(poly.ptr.terms,sizeof(Term)*index);
		setPolySize(poly,index);
	}
	return poly;
}

Poly polySort(unmut Poly poly){
	return _polySort(polyDup(poly));
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
		DIE;
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
	reminder = K2Poly(K_0);
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

