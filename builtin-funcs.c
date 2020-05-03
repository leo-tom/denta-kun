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

int _cmpFunction(const Function *v1,const Function *v2){
	return strcmp(v1->name,v2->name);
}

Poly callBuiltInFunc(const char *name,Poly arg,BlackBoard blackboard){
	Function findThis = {
		.name = {0} //for some reason, valgrind says "Conditional jump or move depends on uninitialised value"
					//without initializing with zero.
	};
	strcpy(findThis.name,name);
	Function *found = bsearch(&findThis,BUILT_IN_FUNCS,BUILT_IN_FUNC_SIZE,sizeof(Function)
						,(int (*)(const void *, const void *))_cmpFunction);
	if(found == NULL){
		fprintf(stderr, "\"%s\" is not defined.\n",name);DIE;
	}
	return found->funcptr(arg,blackboard);
}

Poly builtIn_SSN(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY || polySize(arg) <= 1){
		fprintf(stderr,"SSN takes at least 2 arguments.\n");DIE;
	}
	Poly *array = unwrapPolyArray(arg);
	Poly divisors = mkPolyArray(&array[1],polySize(arg) - 1 );
	Poly result = polyDiv(array[0],divisors); polyFree(arg);
	return result;
}

Poly builtIn_PBB(Poly arg,BlackBoard blackboard){
	printBlackBoard(blackboard,OUTFILE);
	polyFree(arg);
	return nullPoly;
}

Poly builtIn_PP(Poly arg,BlackBoard blackboard){
	polyPrint(arg,K2str,OUTFILE);
	fprintf(OUTFILE,"\\\\\n");
	return arg;
}
Poly builtIn_PPR(Poly arg,BlackBoard blackboard){
	polyPrint(arg,K2str,OUTFILE);
	return arg;
}
Poly builtIn_PPS(Poly arg,BlackBoard blackboard){
	polyPrint(arg,K2strScientific,OUTFILE);
	fprintf(OUTFILE,"\n");
	return arg;
}
Poly builtIn_SP(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY && polySize(arg) != 2){
		fprintf(stderr,"S polynomial can be computed only with 2 polynomials\n");
		DIE;
	}
	Poly *array = unwrapPolyArray(arg);
	Poly retval = polyS(array[0],array[1]);
	polyFree(arg);
	return retval;
}
Poly builtIn_BBA(Poly array,BlackBoard blackboard){
	Poly r;
	size_t size;
	do{
		size = polySize(array);
		r = isThisGrobnerBasis(array);
		#if DEBUG == 1
		polyPrint(array,K2str,stderr);
		fprintf(stderr,"is ");
		#endif
		if(isZeroPoly(r) || polyDegrees(r) == 0){
			#if DEBUG == 1
			fprintf(stderr,"Grobner basis.\n ");
			#endif
			polyFree(r);
			break;
		}
		#if DEBUG == 1
		fprintf(stderr,"not Grobner basis because : \n");
		polyPrint(r,K2str,stderr);
		fprintf(stderr,"is not empty.\n");
		#endif
		polyNice(r);
		size++;
		array.ptr.polies = realloc(array.ptr.polies,size * sizeof(Poly));
		array.ptr.polies[size-1] = r;
		setPolySize(array,size);
	}while(1);
	return GrobnerBasis2ReducedGrobnerBasis(array);
}
Poly builtIn_RED(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY && polySize(arg) != 2){
		DIE;
	}
	Poly *array = unwrapPolyArray(arg);
	Poly divisors;
	if(polyType(array[1]) == ARRAY){
		divisors = polyDup(array[1]);
	}else{
		Poly tmp = polyDup(array[1]);
		divisors = mkPolyArray(&tmp,1);
	}
	Poly result = polySim(array[0],divisors);
	polyFree(arg);
	polyFree(divisors);
	return result;
}
Poly builtIn_SRT(Poly arg,BlackBoard blackboard){
	Poly *array;
	size_t size;
	if(polyType(arg) == ARRAY){
		array = unwrapPolyArray(arg);
		size = polySize(arg);
	}else{
		polyFree(arg);
		return nullPoly;
	}
	int64_t type = poly2Double(array[0]) + 0.5;
	MonomialOrder order;
	switch (type) {
		case MONOMIAL_ORDER_IN_BIN__LEX: {order = LEX;break;}
		case MONOMIAL_ORDER_IN_BIN__RLEX: {order = RLEX;break;}
		case MONOMIAL_ORDER_IN_BIN__PLEX: {order = PLEX;break;}
		default :{
			fprintf(stderr,"Unknown monomial order \"%ld\"\n",type);
			DIE;
		}
	}
	size_t i;
	size--;
	array++;
	Poly *ptr = malloc(size * sizeof(Poly));
	for(i = 0;i < size;i++){
		ptr[i] = polySort(array[i],order);
	}
	Poly retval;
	if(size == 1){
		retval = ptr[0];
		free(ptr);
	}else{
		retval = mkPolyArray(ptr,size);
	}
	polyFree(arg);
	return retval;
}
#include <sys/time.h>
Poly builtIn_CLOCK(Poly arg,BlackBoard blackboard){
	polyFree(arg);
	#if BOOLEAN
	struct timeval tval;
	gettimeofday(&tval,0);
	fprintf(stderr,"%e\n",((double) tval.tv_sec * 1000000.0) + tval.tv_usec);
	return nullPoly;
	#else
	K shifter,ksec,kmic;
	initK(shifter);
	initK(ksec);
	initK(kmic);
	mpq_set_si(shifter,1000000,1);
	struct timeval tval;
	gettimeofday(&tval,0);
	K k;
	initK(k);
	mpq_set_si(ksec,tval.tv_sec,1);
	mpq_set_si(kmic,tval.tv_usec,1);
	mulK(k,ksec,shifter);
	addK(k,k,kmic);
	Poly retval = K2Poly(k,LEX);
	freeK(k);
	freeK(ksec);
	freeK(kmic);
	freeK(shifter);
	return retval;
	#endif
}
Poly builtIn_SUB(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY || polySize(arg) != 2){
		goto err;
	}
	Poly poly = unwrapPolyArray(arg)[0];
	Poly __values = unwrapPolyArray(arg)[1];
	Poly *values;
	size_t size;
	if(polyType(__values) != ARRAY){
		size = 1;
		values = &__values;
	}else{
		size = polySize(__values);
		values = unwrapPolyArray(__values);
	}
	int i,j,k;
	Poly val = polyDup(zeroPoly);
	for(i = 0;i < polySize(poly);i++){
		Poly tmp = K2Poly(poly.ptr.terms[i].coefficient,polyType(poly));
		for(j = 0;j < termSize(poly.ptr.terms[i]);j++){
			if(termDegree(poly.ptr.terms[i],j)){
				if(j >= size){
					goto err;
				}
				for(k = 0;k < termDegree(poly.ptr.terms[i],j);k++){
					Poly temp = polyMul(tmp,values[j]);
					polyFree(tmp);
					tmp = temp;
				}
			}
		}
		Poly temp = polyAdd(val,tmp);
		polyFree(val);
		val = temp;
	}
	polyFree(arg);
	return val;
	err : 
	fprintf(stderr,"builtIn_SUB expects 2 arrays as argument.\n");
	fprintf(stderr,"...or, you did not give me enough values to substitute.\n");
	DIE;
}
extern Poly BCA(Poly arg,BlackBoard blackboard);

Poly builtIn_BCA(Poly arg,BlackBoard blackboard){
	#if !BOOLEAN
	fprintf(stderr,"This function can be used only in boolean mode.");
	DIE;
	#endif
	return BCA(arg,blackboard);
}

extern Poly PAC(Poly arg,BlackBoard blackboard);

Poly builtIn_PAC(Poly arg,BlackBoard blackboard){
	#if !BOOLEAN
	fprintf(stderr,"This function can be used in boolean mode only.");
	DIE;
	#endif
	return PAC(arg,blackboard);
}

Poly builtIn_DIV(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY && polySize(arg) != 2){
		DIE;
	}
	Poly *array = unwrapPolyArray(arg);
	Poly retval = polyDiv(array[0],array[1]);
	polyFree(arg);
	return retval;
}
Poly builtIn_DIE(Poly arg,BlackBoard blackboard){
	polyFree(arg);
	fprintf(stderr,"built-in function \"DIE\" is called.\n");
	fprintf(stderr,"bye bye.\n");
	exit(0);
}
Poly builtIn_IN(Poly arg,BlackBoard blackboard){
	if(polyType(arg) == ARRAY){
		Poly *array = unwrapPolyArray(arg);
		Poly *retval = malloc(sizeof(Poly)*polySize(arg));
		size_t i;
		for(i = 0;i < polySize(arg);i++){
			retval[i] = builtIn_IN(polyDup(array[i]),blackboard);
		}
		polyFree(arg);
		return mkPolyArray(retval,i);
	}else{
		Poly retval = polyIn(arg);
		polyFree(arg);
		return retval;
	}
}
Poly builtIn_RZP(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY){
		if(isZeroPoly(arg)){
			polyFree(arg);
			return nullPoly;
		}
	}else{
		Poly *array = unwrapPolyArray(arg);
		size_t i;
		size_t index = 0;
		for(i = 0;i < polySize(arg);i++){
			if(isNullPoly(array[index++] = builtIn_RZP(array[i],blackboard))){
				index--;
			}
		}
		setPolySize(arg,index);
	}
	return arg;
}
Poly builtIn_EQ(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY || polySize(arg) <= 1){
		fprintf(stderr,"EQ function expects two parameters\n");
		DIE;
	}else{
		Poly *array = unwrapPolyArray(arg);
		size_t i;
		for(i = 0;i + 1 < polySize(arg);i++){
			if(polyType(array[i]) != ARRAY && polyType(array[i + 1]) != ARRAY){
				if(polyType(array[i]) != polyType(array[i + 1])){
					Poly p1 = polySort(array[i],LEX);
					Poly p2 = polySort(array[i+1],LEX);
					if(polyCmp(p1,p2)){
						polyFree(p1);
						polyFree(p2);
						return polyDup(zeroPoly);
					}
					polyFree(p1);
					polyFree(p2);
				}else{
					if(polyCmp(array[i],array[i+1])){
						polyFree(arg);
						return polyDup(zeroPoly);
					}
				}
			}else if(polyType(array[i]) == ARRAY && polyType(array[i + 1]) == ARRAY){
				if(polySize(array[i]) != polySize(array[i + 1])){
						return polyDup(zeroPoly);
				}
				size_t j;
				for(j = 0;j < polySize(array[i]);j++){
					Poly *ptr = malloc(sizeof(Poly) * 2);
					ptr[0] = polyDup(unwrapPolyArray(array[i])[j]);
					ptr[1] = polyDup(unwrapPolyArray(array[i + 1])[j]);
					Poly retval = builtIn_EQ(mkPolyArray(ptr,2),blackboard);
					if(isZeroPoly(retval)){
						polyFree(arg);
						return retval;
					}
					polyFree(retval);
				}
			}else {
				return polyDup(zeroPoly);
			}
		}
	}
	return K2Poly(K_1,LEX);
}

//ABCDEFGHIJKLMNOPQRSTUVWXYZ
const Function BUILT_IN_FUNCS[] = {
	{
		.name = "BBA",
		.description = "Buchberger algorithm",
		.funcptr = builtIn_BBA
	},
	{
		.name = "BCA",
		.description = "Boolean Cellular Automaton",
		.funcptr = builtIn_BCA
	},
	{
		.name = "CLOCK",
		.description = "Call clock() function.",
		.funcptr = builtIn_CLOCK
	},
	{
		.name = "DIE",
		.description = "Die.",
		.funcptr = builtIn_DIE
	},
	{
		.name = "DIV",
		.description = "Divide polynomial.",
		.funcptr = builtIn_DIV
	},
	{
		.name = "EQ",
		.description = "True if Equal",
		.funcptr = builtIn_EQ
	},
	{
		.name = "IN",
		.description = "Return initial ideal or initial polynomial.",
		.funcptr = builtIn_IN
	},
	{
		.name = "PAC",
		.description = "Convert array of polynomials to C code.",
		.funcptr = builtIn_PAC
	},
	{
		.name = "PBB",
		.description = "Print out blackboard.",
		.funcptr = builtIn_PBB
	},
	{
		.name = "PP",
		.description = "Print given polynomials.",
		.funcptr = builtIn_PP
	},
	{
		.name = "PPR",
		.description = "Same as PP. But does not print out \\\\ and \\n",
		.funcptr = builtIn_PPR
	},
	{
		.name = "PPS",
		.description = "Print given polynomials with scientific notation.",
		.funcptr = builtIn_PPS
	},
	{
		.name = "RED",
		.description = "Reduce polynomial",
		.funcptr = builtIn_RED
	},
	{
		.name = "RZP",
		.description = "Remove Zeros in array.",
		.funcptr = builtIn_RZP
	},
	{
		.name = "SP",
		.description = "Calculate S polynomial",
		.funcptr = builtIn_SP
	},
	{
		.name = "SRT",
		.description = "Sort polynomial by specified monomial order",
		.funcptr = builtIn_SRT
	},
	{
		.name = "SSN",
		.description = "Calculate standard notation.",
		.funcptr = builtIn_SSN
	},
	{
		.name = "SUB",
		.description = "Substitute values",
		.funcptr = builtIn_SUB
	}	
};
const size_t BUILT_IN_FUNC_SIZE = sizeof(BUILT_IN_FUNCS)/sizeof(Function);

