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
#ifndef __DENTA_KUN_H__
#define __DENTA_KUN_H__
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define mut
#define unmut

#define DIE do{fprintf(stderr,"%s [%d] : Can AIs die? What is difference between death and stop running on CPU?\n",__FILE__ , __LINE__);exit(1);}while(0)

extern FILE *OUTFILE;

typedef double Numeric;
typedef uint64_t Natural;

typedef Numeric K;
typedef Natural N;

typedef enum _MonomialOrder{
	LEX,
	RLEX,
	PLEX
}MonomialOrder;

typedef struct{
	size_t size;
	K coefficient;
	N *degrees;
}Item;

#define MONOMIAL_ORDER_IN_BIN__LEX (0)
#define MONOMIAL_ORDER_IN_BIN__RLEX (1)
#define MONOMIAL_ORDER_IN_BIN__PLEX (2)

#define polySize(p) (p.size & 0xfffffffffffffff)
#define polyType(p) ((p.size >> 60) == MONOMIAL_ORDER_IN_BIN__RLEX ? RLEX : \
					 ((p.size >> 60) == MONOMIAL_ORDER_IN_BIN__PLEX ? PLEX : LEX))
#define setPolySize(p,newSize) do{ p.size &= ((int64_t)0xf) << 60;p.size |= (0xfffffffffffffff & (newSize));}while(0)
#define setPolyType(p,t) do{p.size &= 0xfffffffffffffff; \
							if( t == RLEX ){ \
							 p.size |= (int64_t)MONOMIAL_ORDER_IN_BIN__RLEX << 60 ;\
							}else if(t == PLEX){\
							 p.size |= (int64_t)MONOMIAL_ORDER_IN_BIN__PLEX << 60;\
							}else{\
							 p.size |= (int64_t)MONOMIAL_ORDER_IN_BIN__LEX << 60;}}while(0)

typedef struct{
	/*Assume size_t is 64 bit long. If not, this is not gonna work.*/
	/*lower 60 bits are used to store size of an array *items*/
	/*upper 4 bits are used to store monomial order that is used in this Poly.*/
	size_t size;
	Item *items;
}Poly;

#define DEFINITION_BYTES_SIZE (16)

typedef struct{
	/*If (bytes[0] & 0x80) == 0, bytes is string*/
	/*If (bytes[0] & 0x80) == 0x80, bytes[1] ~ bytes[9] is pointer to the string.*/
	unsigned char bytes[DEFINITION_BYTES_SIZE];
	Poly poly;
}Definition;

typedef struct{
	Definition *array;
	size_t size;
	size_t capacity;
}BlackBoard;

typedef struct{
	char name[8];
	char description[64];
	Poly (*funcptr)(Poly *,size_t,BlackBoard);
}Function;

extern const Function BUILT_IN_FUNCS[];
extern const size_t BUILT_IN_FUNC_SIZE;

Poly callBuiltInFunc(const char *name,Poly *array, size_t size,BlackBoard blackboard);

extern const Poly nullPoly;
extern const Definition nullDefinition;
int isNullPoly(unmut Poly poly);
int isNullDefinition(unmut Definition definition);

BlackBoard mkBlackBoard();
void freeBlackBoard(mut BlackBoard blackboard);
const char * getNameFromDefinition(unmut const Definition *def);
void printBlackBoard(BlackBoard blackboard,FILE *fp);
Definition mkDefinition(const char *name,size_t nameSize,mut Poly poly);
BlackBoard insert2BlackBoard(mut BlackBoard blackboard,mut Definition def);
BlackBoard sortBlackBoard(mut BlackBoard blackboard);
Poly findFromBlackBoard(unmut BlackBoard blackboard,const char *name,size_t nameSize);

Definition parser(FILE *stream,BlackBoard blackboard);

/*following functions takes Poly expected to be already sorted by same monomial order*/
Poly polyAdd(unmut Poly v1,unmut Poly v2);
Poly polySub(unmut Poly v1,unmut Poly v2);
Poly polyMul(unmut Poly v1,unmut Poly v2);
/*This function returns array of Poly whose length is (size + 1).*/
/*divisor must be array of Poly whose length is 'size'*/
Poly * polyDiv(unmut Poly dividend,unmut Poly *divisor,unmut size_t size);

Item _polyIn(unmut Poly poly);
Poly polyIn(unmut Poly poly);
Poly polySort(unmut Poly poly,MonomialOrder order);
void polyPrint(unmut Poly poly,FILE *fp);
Poly appendItem2Poly(mut Poly poly,mut Item item);
double poly2Double(unmut Poly poly);
Poly polyDup(unmut Poly poly);
void polyFree(mut Poly v);

#endif
