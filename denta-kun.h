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
#include <dlfcn.h>

#include <gmp.h>

#define mut
#define unmut

#define DIE do{fprintf(stderr,"Error at %s:%d\n",__FILE__ , __LINE__);exit(1);}while(0)

#ifdef __GNUC__
#define BUILTIN_POPCOUNT_EXISTS
#endif
#ifdef __clang__
#define BUILTIN_POPCOUNT_EXISTS
#endif

#ifdef BUILTIN_POPCOUNT_EXISTS
#define popcount(v) (sizeof(v) > 4 ? __builtin_popcountll(v) : __builtin_popcount(v))
#endif

#ifndef BUILTIN_POPCOUNT_EXISTS
#error popcount not defined
#endif

#if !(RATIONAL || BOOLEAN)
#define RATIONAL (1)
#endif

#if RATIONAL
typedef mpq_t Q; 
typedef Q Numeric;
#elif BOOLEAN
typedef unsigned char B;
typedef B Numeric;
#else
#error Nope. 
#endif

typedef uint64_t Natural;

typedef Numeric K;
typedef Natural N;

extern FILE *OUTFILE;
extern size_t SUBSHIFT;
extern void *LOADED_FUNCTION_PTR;
extern size_t LOADED_FUNCTION_INPUT_SIZE;
extern size_t LOADED_FUNCTION_OUTPUT_SIZE;

extern K JOHO_NO_TANIGEN; //1
extern K KAHO_NO_TANIGEN; //0
extern K JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN; //-1
void initConst();
#define K_1 JOHO_NO_TANIGEN
#define K_0 KAHO_NO_TANIGEN 
#define K_N1 JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN


//Free return value of these 2.
char * K2str(K k);
char * K2strScientific(K k);

#if RATIONAL

void str2K(K val,const char *str);
void addK(K val,const K v1,const K v2);
void subK(K val,const K v1,const K v2);
void mulK(K val,const K v1,const K v2);
void divK(K val,const K v1,const K v2);
#define K2double(k) mpq_get_d(k)
#define cmpK(v1,v2) mpq_cmp(v1,v2)
#define freeK(k) mpq_clear(k)
#define copyK(to,from) (mpq_init(to), mpq_set(to,from),to)
#define initK(k) (mpq_init(k),k)

#elif BOOLEAN
#define addK(val,v1,v2) (val = (v1 & 0x1) ^ (v2 & 0x1))
#define subK(val,v1,v2) (val = (v1 & 0x1) ^ (v2 & 0x1))
#define mulK(val,v1,v2) (val = (v1 & v2) & 0x1)
#define divK(val,v1,v2) (val = (v1 == 0) ? 0 : 1 )
#define str2K(val,str) ( atoi(str) ? (val = 1) : (val = 0) )
#define K2double(k) ((double) k)
#define cmpK(v1,v2) (v1 == v2 ? 0 : (v1 > v2 ? 1 : -1))
#define freeK(k) (0)
#define copyK(to,from) (to = from)
#define initK(k) (k=0)

#else
#error Nope. 
#endif

typedef enum _MonomialOrder{
	LEX,
	RLEX,
	PLEX,
	PRLEX,
	ARRAY
}MonomialOrder;

typedef struct{
	uint16_t sizu; 
	K coefficient;
	union{
		N *ptr;
		N val;
	}deg;
}Term;

typedef struct __Poly__{
	/*Assume size_t is 64 bit long. If not, this is not gonna work.*/
	/*lower 60 bits are used to store size of an array *items*/
	/*upper 4 bits are used to store monomial order that is used in this Poly.*/
	size_t size;
	union{
		Term *terms;
		struct __Poly__ *polies;
	}ptr;
}Poly;

#define MONOMIAL_ORDER_IN_BIN__LEX (0)
#define MONOMIAL_ORDER_IN_BIN__RLEX (1)
#define MONOMIAL_ORDER_IN_BIN__PLEX (2)
#define MONOMIAL_ORDER_IN_BIN__PRLEX (3)
#define MONOMIAL_ORDER_IN_BIN__ARRAY (4)

void _setPolySize(Poly *poly,size_t size);
void _setPolyType(Poly *poly,MonomialOrder);

#define polySize(p) ((p).size & 0xfffffffffffffff)
#define polyType(p) ((p).size >> 60 == MONOMIAL_ORDER_IN_BIN__RLEX ? RLEX : ( \
					 (p).size >> 60 == MONOMIAL_ORDER_IN_BIN__PLEX ? PLEX : ( \
					 (p).size >> 60 == MONOMIAL_ORDER_IN_BIN__PRLEX ? PRLEX : ( \
					 (p).size >> 60 == MONOMIAL_ORDER_IN_BIN__LEX ? LEX : ARRAY))))

#define setPolySize(polynomial,newSize) _setPolySize(&polynomial,newSize)
#define setPolyType(polynomial,typeToBeSet) _setPolyType(&polynomial,typeToBeSet)

#if BOOLEAN
#define termSize(term) (term.sizu)
#define termFree(term) do{ if(term.sizu <= sizeof(N)*8){term.deg.val = 0;}else{free(term.deg.ptr);/*freeK(term.coefficient);*/}}while(0)
#define termDegree(term,index) ((term.sizu <= sizeof(N)*8) \
									? ((term.deg.val >> index) & 0x1) \
									: (N)((term.deg.ptr[index/(sizeof(N)*8)] >> (index % (sizeof(N)*8))) & 0x1))
#define termDegreeAllocator(term) ((term.sizu <= sizeof(N)*8) ? (term.deg.val = 0,NULL) \
									: (term.deg.ptr = calloc(sizeof(N),(term.sizu-1)/(sizeof(N)*8) + 1)))
#define setTermDegree(term,index,value) (value ? \
											((term.sizu <= sizeof(N)*8) \
												? (term.deg.val |= (1 << index))\
												: (term.deg.ptr[index/(sizeof(N)*8)] |= (1 << (index % (sizeof(N)*8)))) \
											)\
											:((term.sizu <= sizeof(N)*8) \
												? (term.deg.val &= ~(1 << index))\
												: ((term.deg.ptr[index/(sizeof(N)*8)] &= (~(1 << (index % (sizeof(N)*8)))))) \
											))
											
#define setTermSize(term,size) (term.sizu = size)
#elif RATIONAL
#define termSize(term) (term.sizu)
#define termDegree(term,index) (term.deg.ptr[index])
#define termDegreeAllocator(term) (term.deg.ptr = calloc(sizeof(N),term.sizu))
#define termFree(term) do{free(term.deg.ptr);}while(0)
#define setTermDegree(term,index,value) (term.deg.ptr[index] = value)
#define setTermSize(term,size) (term.sizu = size)
#else
#error Nope.
#endif


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
Poly cfunc2Poly(size_t inputSize,size_t outputSize,BlackBoard blackboard);

extern const Poly nullPoly;
extern Poly zeroPoly;
extern Definition nullDefinition;
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

Poly parser(FILE *stream,BlackBoard *blackboard);
Poly instantParser(char *code,BlackBoard *blackboard);

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
int polyCmp(unmut Poly v1,unmut Poly v2);

Poly K2Poly(mut K k,MonomialOrder order);
#define poly2K(k,poly) ((polyType(poly) == ARRAY || polySize(poly) != 1 || termSize(poly.ptr.terms[0]) != 0) ? \
		(fprintf(stderr, "Unable to call poly2K on : "),polyPrint(poly,K2str,stderr),fprintf(stderr,"\n"),exit(1),k) : \
		copyK(k,poly.ptr.terms[0].coefficient))
int isZeroPoly(unmut Poly poly);
N polyDegrees(unmut Poly p);
Term __polyIn(unmut Poly poly);
Term _polyIn(unmut Poly poly);
Term dupTerm(unmut Term);
int cmpTerm(MonomialOrder order,Term v1,Term v2);
Poly polyIn(unmut Poly poly);
Poly term2Poly(mut Term);
Poly polySort(unmut Poly poly,MonomialOrder order);
void polyPrint(unmut Poly poly,char*(*printer)(K ),FILE *fp);
//Poly appendTerm2Poly(mut Poly poly,mut Term);
double poly2Double(unmut Poly poly);
Poly polyDup(unmut Poly poly);
void polyFree(mut Poly v);
Poly mkPolyArray(mut Poly *array,size_t size);
Poly * unwrapPolyArray(mut Poly poly);

Poly isThisGrobnerBasis(Poly array);
Poly GrobnerBasis2ReducedGrobnerBasis(mut Poly grobner);
Poly removeUnnecessaryPolies(Poly grobner);

#endif
