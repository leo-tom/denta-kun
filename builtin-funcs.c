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
		polyPrint(array,stdout);
		printf("is ");
		if(isZeroPoly(r)){
			printf("Grobner basis.\n ");
			polyFree(r);
			break;
		}
		printf("not Grobner basis because : \n");
		polyPrint(r,stdout);
		printf("is not empty.\n");
		if(i >= capacity){
			capacity *= 2;
			array.ptr.polies = realloc(array.ptr.polies,capacity * sizeof(Poly));
		}
		array.ptr.polies[i] = r;
		i++;
		setPolySize(array,i);
	}while(1);
	if(i < capacity){
		array.ptr.polies = realloc(array.ptr.polies,i * sizeof(Poly));
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
	Poly result = polySim(array[0],divisors); polyFree(arg);
	return result;
}
const size_t BUILT_IN_FUNC_SIZE = 6;
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
		.name = "SSN",
		.description = "Calculate standard notation.",
		.funcptr = builtIn_SSN
	}
	
};

