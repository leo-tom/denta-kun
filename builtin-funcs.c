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

Poly callBuiltInFunc(const char *name,Poly *array, size_t size,BlackBoard blackboard){
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
	return found->funcptr(array,size,blackboard);
}

Poly builtIn_SSN(Poly *array,size_t size,BlackBoard blackboard){
	if(size <= 1){
		fprintf(stderr,"SSN takes at least 2 arguments.\n");DIE;
	}
	Poly *result = polyDiv(array[0],&array[1],size - 1);
	size_t i;
	#if DEBUG
		FILE *fp = stderr;
		polyPrint(array[0],fp);
		fprintf(fp, " = ");
		array++;
		size--;
		for(i = 0;i < size;i++){
			fprintf(fp," (");
			polyPrint(result[i],fp);
			fprintf(fp,")(");
			polyPrint(array[i],fp);
			fprintf(fp,") +");
		}
		size++;
		fprintf(fp," ");
		polyPrint(result[i],fp);
		fprintf(fp,"\n");
		array--;
	#endif
	polyFree(result[0]);
	polyFree(result[size - 1]);
	for(i = 1;i < size;i++){
		polyFree(array[i]);
		polyFree(result[i]);
	}
	return array[0];
}

Poly builtIn_PBB(Poly *array,size_t size,BlackBoard blackboard){
	printBlackBoard(blackboard,OUTFILE);
	size_t i;
	for(i = 0;i < size;i++){
		polyFree(*array++);
	}
	return nullPoly;
}

Poly builtIn_PP(Poly *array,size_t size,BlackBoard blackboard){
	size_t i;
	for(i = 0;i < size;i++){
		polyPrint(*array,OUTFILE);
		polyFree(*array++);
	}
	return nullPoly;
}

const size_t BUILT_IN_FUNC_SIZE = 3;
const Function BUILT_IN_FUNCS[] = {
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
		.name = "SSN",
		.description = "Print standard notation to stderr.",
		.funcptr = builtIn_SSN
	}
};

