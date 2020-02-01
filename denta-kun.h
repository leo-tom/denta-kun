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
#ifndef __DENTA_KUN_H__
#define __DENTA_KUN_H__
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <gmp.h>

#define mut
#define unmut

#define DIE do{fprintf(stderr,"%s [%d] : Can AIs die? What is difference between death and stop running on CPU?\n",__FILE__ , __LINE__);exit(1);}while(0)

extern FILE *OUTFILE;


typedef mpq_t Q; 

typedef Q Numeric;
//typedef uint64_t Natural;
typedef int64_t Natural;

typedef Numeric K;
typedef Natural N;
extern K JOHO_NO_TANIGEN; //1
extern K KAHO_NO_TANIGEN; //0
extern K JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN; //-1
void initConst();
#define K_1 JOHO_NO_TANIGEN
#define K_0 KAHO_NO_TANIGEN 
#define K_N1 JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN

int cmpK(const K v1,const K v2);
void str2K(K val,const char *str);
char * K2str(K k,char *buff);
char * K2strScientific(K k,char *buff);
double K2double(const K k);
void addK(K val,const K v1,const K v2);
void subK(K val,const K v1,const K v2);
void mulK(K val,const K v1,const K v2);
void divK(K val,const K v1,const K v2);
void freeK(K k);
void copyK(K to,const K from);
void initK(K k);

typedef enum _MonomialOrder{
	LEX,
	RLEX,
	PLEX,
	ARRAY
}MonomialOrder;

typedef struct{
	size_t size;
	K coefficient;
	N *degrees;
}Item;

typedef struct __Poly__{
	/*Assume size_t is 64 bit long. If not, this is not gonna work.*/
	/*lower 60 bits are used to store size of an array *items*/
	/*upper 4 bits are used to store monomial order that is used in this Poly.*/
	size_t size;
	union{
		Item *items;
		struct __Poly__ *polies;
	}ptr;
}Poly;

#define MONOMIAL_ORDER_IN_BIN__LEX (0)
#define MONOMIAL_ORDER_IN_BIN__RLEX (1)
#define MONOMIAL_ORDER_IN_BIN__PLEX (2)
#define MONOMIAL_ORDER_IN_BIN__ARRAY (3)

void _setPolySize(Poly *poly,size_t size);
void _setPolyType(Poly *poly,MonomialOrder);

#define polySize(p) (p.size & 0xfffffffffffffff)
#define polyType(p) ((p.size >> 60) == MONOMIAL_ORDER_IN_BIN__RLEX ? RLEX : \
					 ((p.size >> 60) == MONOMIAL_ORDER_IN_BIN__PLEX ? PLEX : ( \
					  (p.size >> 60) == MONOMIAL_ORDER_IN_BIN__LEX ? LEX : ARRAY)))

#define setPolySize(polynomial,newSize) _setPolySize(&polynomial,newSize)
#define setPolyType(polynomial,typeToBeSet) _setPolyType(&polynomial,typeToBeSet)


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
	Poly (*funcptr)(Poly ,BlackBoard);
}Function;

extern const Function BUILT_IN_FUNCS[];
extern const size_t BUILT_IN_FUNC_SIZE;

Poly callBuiltInFunc(const char *name,Poly arg,BlackBoard blackboard);

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
Poly polyDiv(unmut Poly dividend,unmut Poly divisor);
Poly polySim(unmut Poly dividend,unmut Poly divisors);
Poly polyS(unmut Poly f,unmut Poly g);
void polyNice(unmut Poly p);

Poly K2Poly(mut K k,MonomialOrder order);
int isZeroPoly(unmut Poly poly);
N polyDegrees(unmut Poly p);
Item __polyIn(unmut Poly poly);
Item _polyIn(unmut Poly poly);
Poly polyIn(unmut Poly poly);
Poly item2Poly(mut Item item);
Poly polySort(unmut Poly poly,MonomialOrder order);
void polyPrint(unmut Poly poly,char*(*printer)(K , char *),FILE *fp);
Poly appendItem2Poly(mut Poly poly,mut Item item);
double poly2Double(unmut Poly poly);
Poly polyDup(unmut Poly poly);
void polyFree(mut Poly v);
Poly mkPolyArray(mut Poly *array,size_t size);
Poly * unwrapPolyArray(mut Poly poly);

Poly isThisGrobnerBasis(unmut Poly array);
Poly GrobnerBasis2ReducedGrobnerBasis(mut Poly grobner);
Poly removeUnnecessaryPolies(Poly grobner);

#endif
