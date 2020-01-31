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
	polyPrint(arg,OUTFILE);
	fprintf(OUTFILE,"\n");
	polyFree(arg);
	return nullPoly;
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
	size_t capacity = polySize(array);
	size_t i = polySize(array);
	do{
		r = isThisGrobnerBasis(array);
		#if DEBUG == 1
		polyPrint(array,stdout);
		printf("is ");
		#endif
		if(isZeroPoly(r) || polyDegrees(r) == 0){
			#if DEBUG == 1
			printf("Grobner basis.\n ");
			#endif
			polyFree(r);
			fprintf(stderr,"YEY! I found Grobner basis\n");
			break;
		}
		#if DEBUG == 1
		printf("not Grobner basis because : \n");
		polyPrint(r,stdout);
		printf("is not empty.\n");
		#endif
		fprintf(stderr,"Nope\n");
		if(i >= capacity){
			capacity *= 2;
			array.ptr.polies = realloc(array.ptr.polies,capacity * sizeof(Poly));
		}
		array.ptr.polies[i] = r;
		i++;
		setPolySize(array,i);
	}while(1);
	if(i < capacity){
		array.ptr.polies = realloc(array.ptr.polies,i * sizeof(array.ptr.polies[0]));
	}
	setPolySize(array,i);
	return GrobnerBasis2ReducedGrobnerBasis(array);
}
Poly builtIn_SIM(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY && polySize(arg) != 2){
		DIE;
	}
	Poly *array = unwrapPolyArray(arg);
	Poly divisors = mkPolyArray(&array[1],polySize(arg) - 1 );
	Poly result = polySim(array[0],divisors);
	polyFree(arg);
	return result;
}
Poly builtIn_SRT(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY){
		DIE;
	}
	Poly *array = unwrapPolyArray(arg);
	int64_t type = poly2Double(*array) + 0.5;
	MonomialOrder order;
	switch (type) {
		case MONOMIAL_ORDER_IN_BIN__LEX: {order = LEX;break;}
		case MONOMIAL_ORDER_IN_BIN__RLEX: {order = RLEX;break;}
		case MONOMIAL_ORDER_IN_BIN__PLEX: {order = PLEX;break;}
		default :{
			fprintf(stderr,"Unknown type \"%ld\"\n",type);
			DIE;
		}
	}
	size_t i;
	size_t size = polySize(arg);
	size--;
	array++;
	Poly *ptr = malloc(size * sizeof(Poly));
	for(i = 0;i < size;i++){
		Poly p = array[i];
		ptr[i] = polySort(p,order);
	}
	Poly retval = nullPoly;
	if(size == 1){
		retval = ptr[0];
		free(ptr);
	}else if (size > 1){
		retval = mkPolyArray(ptr,size);
	}
	polyFree(arg);
	return retval;
}

//ABCDEFGHIJKLMNOPQRSTUVWXYZ
const size_t BUILT_IN_FUNC_SIZE = 7;
const Function BUILT_IN_FUNCS[] = {
	{
		.name = "BBA",
		.description = "Buchberger algorithm",
		.funcptr = builtIn_BBA
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
		.name = "SP",
		.description = "Calculate S polynomial",
		.funcptr = builtIn_SP
	},
	{
		.name = "SIM",
		.description = "Calculate simplified polynomial",
		.funcptr = builtIn_SIM
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
	}
	
};

