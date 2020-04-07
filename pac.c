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

#if BOOLEAN

typedef struct _Container{
	int number;
	uint64_t *values; //values[1] == 0x3 means it has x_0 and x_1 at 1st bit(not 0th)
	struct _Container *next;
	void *instruction;
}Container;

typedef enum{
	AND,
	XOR
}Operation;

typedef struct{
	Container *c1;
	Container *c2;
	Operation op;
}Instruction;

Container * PAC_AND(uint64_t *targets,size_t targetSize,size_t inputSize,size_t outputSize){
	int i,j;
	Container *container = NULL;
	for(i = 0;i < inputSize;i++){
		Container *new = calloc(sizeof(Container),1);
		new->number = i;
		new->next = container;
		new->values = malloc(sizeof(uint64_t) * inputSize);
		new->instruction = NULL;
		for(j = 0;j < inputSize;j++){
			new->values[(i+j) % inputSize] = 1 << j;
		}
		container = new;
	}
	size_t currentSize = targetSize;
	for(i = 0;i < targetSize;i++){
		if(popcount(targets[i]) == 1){
			targets[i] = 0;
			currentSize--;
		}
	}
	while(currentSize > 0){
		Container *best1,*best2,*ok1,*ok2;
		uint64_t *bestValue = NULL;
		int64_t bestScore = 0;
		best1 = best2 = ok1 = ok2 = NULL;
		uint64_t *okValue = NULL;
		int64_t okScore = 0;
		Container *c1 = container;
		while(c1){
			Container *c2 = c1->next;
			while(c2){
				uint64_t *newVal = malloc(sizeof(uint64_t) * inputSize);
				for(i = 0;i < inputSize;i++){
					newVal[i] = c1->values[i] | c2->values[i];
				}
				int64_t score = 0;
				int64_t _okScore = 0;
				for(i = 0;i < targetSize;i++){
					if(targets[i] == 0){
						continue;
					}
					for(j = 0;j < inputSize;j++){
						if(targets[i] == newVal[j]){
							score++;
							break;
						}else{
							int64_t positive = targets[i] & newVal[j];
							int64_t negative = ( ~ targets[i] ) & newVal[j];
							_okScore += popcount(positive) - popcount(negative); 
						}
					}
				}
				if(score > bestScore){
					bestScore = score;
					best1 = c1;
					best2 = c2;
					bestValue = newVal;
				}else if(bestScore == 0 && _okScore > okScore){
					okScore = _okScore;
					ok1 = c1;
					ok2 = c2;
					okValue = newVal;
				}else{
					free(newVal);
				}
				c2 = c2->next;
			}
			c1 = c1->next;
		}
		if(bestScore <= 0){
			bestScore = okScore;
			best1 = ok1;
			best2 = ok2;
			bestValue = okValue;
		}
		if(bestScore > 0){
			Container *c = malloc(sizeof(Container));
			c->number = container->number + 1;
			c->next = container;
			c->values = bestValue;
			c->instruction = malloc(sizeof(Instruction));
			((Instruction *)c->instruction)->c1 = best1;
			((Instruction *)c->instruction)->c2 = best2;
			((Instruction *)c->instruction)->op = AND;
			container = c;
			for(i = 0;i < targetSize;i++){
				if(targets[i] == 0){
					continue;
				}
				for(j = 0;j < inputSize;j++){
					if(targets[i] == c->values[j]){
						targets[i] = 0;
						currentSize--;
					}
				}
				 
			}
		}else{
			fprintf(stderr,"NO!\n");
			DIE;
		}
	}
	return container;
}

uint64_t term2uint64_t(Term term){
	int i;
	uint64_t retval = 0;
	for(i = 0;i < termSize(term);i++){
		if(termDegree(term,i)){
			retval |= 1 << i;
		}
	}
	return retval;
}

void printAllContainers(Container *container,size_t inputSize){
	if(container == NULL){
		return;
	}
	printAllContainers(container->next,inputSize);
	if(container->instruction == NULL){
		fprintf(OUTFILE,"\tconst uint64_t v%d = (v << %d) | (v >> %ld);\n",container->number,container->number,inputSize - container->number);
	}else{
		fprintf(OUTFILE,"\tconst uint64_t v%d = v%d & v%d;\n"
		,container->number,((Instruction *)container->instruction)->c1->number,((Instruction *)container->instruction)->c2->number);
	}
}

Poly PAC(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY){
		fprintf(stderr,"PAC expects array whose size is 2^n.\n");
		DIE;
	}
	int64_t i,j,k;
	Poly *array = unwrapPolyArray(arg);
	size_t termsSize = 0;
	size_t inputSize = 0;
	size_t outputSize = polySize(arg);
	if(popcount(outputSize) != 1 || outputSize == 1){
		fprintf(stderr,"PAC expects array whose size is 2^n. But is %lu\n",outputSize);
		DIE;
	}
	for(i = 0;i < outputSize;i++){
		termsSize += polySize(array[i]);
		for(j = 0;j < polySize(array[i]);j++){
			if(termSize(array[i].ptr.terms[j]) > inputSize){
				inputSize = termSize(array[i].ptr.terms[j]);
			}
		}
	}
	uint64_t *terms = malloc(sizeof(uint64_t) * termsSize);
	size_t *indexes = calloc(sizeof(size_t) , outputSize);
	size_t remaining = outputSize;
	size_t termsIndex = 0;
	MonomialOrder order = polyType(array[0]);
	while(remaining > 0){
		Term biggestTerm;
		int first = 1;
		for(i = 0;i < outputSize;i++){
			if(indexes[i] >= polySize(array[i])){
				continue;
			}
			if(first){
				biggestTerm = array[i].ptr.terms[indexes[i]];
				first = 0;
			}else if(cmpTerm(order,biggestTerm,array[i].ptr.terms[indexes[i]]) < 0){
				biggestTerm = array[i].ptr.terms[indexes[i]];
			}
		}
		for(i = 0;i < outputSize;i++){
			if(indexes[i] >= polySize(array[i])){
				continue;
			}
			if(cmpTerm(order,biggestTerm,array[i].ptr.terms[indexes[i]]) == 0){
				if((++indexes[i]) >= polySize(array[i])){
					remaining--;
				}
			}
		}
		if(termSize(biggestTerm) != 0){
			terms[termsIndex++] = term2uint64_t(biggestTerm);
		}
	}
	Container *container = PAC_AND(terms,termsIndex,inputSize,outputSize);
	free(terms);
	fprintf(OUTFILE,"uint64_t functionName(uint64_t v){\n");
	for(i = 0;i < outputSize;i++){
		fprintf(OUTFILE,"/*\nf%ld = ",i);
		polyPrint(array[i],K2str,OUTFILE);
		fprintf(OUTFILE,"\\\\\n*/\n");
	}
	printAllContainers(container,inputSize);
	fprintf(OUTFILE,"\treturn ");
	for(i = 0;i < outputSize;i++){
		Poly poly = array[i];
		fprintf(OUTFILE,"((");
		for(j = 0;j < polySize(poly);j++){
			Term term = poly.ptr.terms[j];
			if(termSize(term) == 0){
				//constant term
				if(!cmpK(term.coefficient,K_1)){
					fprintf(OUTFILE,"(1 << %ld)) & (1 << %ld))",i,i);
				}else{
					fprintf(OUTFILE,"0) & (1 << %ld))",i);
				}
				continue;
			}
			uint64_t termValue = term2uint64_t(term);
			Container *now = container;
			Container *useThis = NULL;
			int64_t nthBit;
			while(now){
				for(k = 0;k < inputSize;k++){
					if(now->values[k] == termValue){
						if(k == i){
							useThis = now;
							nthBit = k;
							goto done;
						}
						useThis = now;
						nthBit = k;
					}
				}
				now = now->next;
			}
			if(useThis == NULL){
				fprintf(stderr,"there must be some bugs at PAC_AND\n");
				DIE;
			}
			done:
			if(i < nthBit){
				fprintf(OUTFILE,"(v%d >> %ld)",useThis->number,nthBit - i);
			}else if(i > nthBit){
				fprintf(OUTFILE,"(v%d << %ld)",useThis->number,i - nthBit);
			}else{
				fprintf(OUTFILE,"v%d",useThis->number);
			}
			if(j + 1 == polySize(poly)){
				fprintf(OUTFILE,") & (1 << %ld))",i);
			}else{
				fprintf(OUTFILE," ^ ");
			}
		}
		if(i + 1 != outputSize){
			fprintf(OUTFILE,"\n\t\t|");
		}
	}
	fprintf(OUTFILE,";\n}\n");
	polyFree(arg);
	return nullPoly;
}
#endif
