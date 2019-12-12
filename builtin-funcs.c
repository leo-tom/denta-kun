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

/*Returns 0 if array is grobner basis*/
/*Returns reminder of S(g_i,g_j) divided by array*/
Poly _isThisGrobnerBasis(Poly array){
	size_t s = polySize(array);
	Poly *ptr = (Poly *) unwrapPolyArray(array);
	Item *in = malloc(sizeof(Item)*s);
	size_t i,j;
	for(i = 0;i < s;i++){
		in[i] = __polyIn(ptr[i]);
	}
	Poly reminder;
	for(i = 0;i < s;i++){
		for(j = i + 1;j < s;j++){
			if(_isCoprime(in[i],in[j])){
				break;
			}
			Poly s_i_j = polyS(ptr[i],ptr[j]);
			reminder = polySim(s_i_j,array);polyFree(s_i_j);
			if(polySize(reminder) == 1 && cmpK(reminder.items[0].coefficient, K_0)){
				polyFree(reminder);
			}else{
				goto ret;
			}
			Poly s_j_i = polyS(ptr[j],ptr[i]);
			reminder = polySim(s_j_i,array);polyFree(s_j_i);
			if(polySize(reminder) == 1 && cmpK(reminder.items[0].coefficient, K_0)){
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
	free(in);
	return reminder;
}
Poly _grobnerBasis2ReducedGrobnerBasis(mut Poly grobner){
	size_t i;
	size_t size = polySize(grobner);
	Poly *ptr = unwrapPolyArray(grobner);
	ptr = realloc(ptr , size * 2 * sizeof(Poly));
	
	for(i = 0;i < size;i++){
		Poly divisors = mkPolyArray(&ptr[i+1],size - 1);
		Poly r = polySim(ptr[i],divisors);
		ptr[i + size] = r;
	}
	Poly *newArray = malloc(sizeof(Poly)*size);
	size_t index = 0;
	for(i = 0;i < size;i++){
		polyFree(ptr[i]);
		Poly p = ptr[i + size];
		if(p.size == 1 && !cmpK(p.items[0].coefficient , K_0)){
			polyFree(p);
			continue;
		}else{
			size_t j;
			K gyaku = divK(K_1,p.items[0].coefficient);
			for(j = 0;j < polySize(p);j++){
				p.items[j].coefficient = mulK(gyaku,p.items[j].coefficient);
			}
			newArray[index++] = p;
		}
	}
	free(ptr);
	if(size != index){
		size = index;
		newArray = realloc(newArray,sizeof(Poly) * size);
	}
	return mkPolyArray(newArray,size);
}
Poly builtIn_BBA(Poly array,BlackBoard blackboard){
	Poly r;
	size_t capacity = polySize(array);
	size_t i = polySize(array);
	while((r=_isThisGrobnerBasis(array)).size >= 1 && !cmpK(r.items[0].coefficient,K_0)){
		if(i >= capacity){
			capacity *= 2;
			array.items = realloc(array.items,capacity * sizeof(Poly));
		}
		((Poly *)array.items)[i] = r;
		i++;
	}
	if(i < capacity){
		array.items = realloc(array.items,i * sizeof(Poly));
	}
	setPolySize(array,i);
	return _grobnerBasis2ReducedGrobnerBasis(array);
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

