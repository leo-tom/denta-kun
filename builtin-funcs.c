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
	#if DEBUG >= 2
	fprintf(stderr, "Calling \"%s\" with : ",name);
	polyPrint(arg,K2str,stderr);
	fprintf(stderr,"\n");
	#endif
	
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
Poly builtIn_PPP(Poly arg,BlackBoard blackboard){
	polyPrintBori(arg,OUTFILE);
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
		#if DEBUG >= 2
		polyPrint(array,K2str,stderr);
		fprintf(stderr,"is ");
		#endif
		if(isZeroPoly(r) || polyDegrees(r) == 0){
			#if DEBUG >= 2
			fprintf(stderr,"a Grobner basis.\n ");
			#endif
			polyFree(r);
			break;
		}
		#if DEBUG >= 2
		fprintf(stderr,"not a Grobner basis because : \n");
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
	Poly result = polySim(array[0],array[1]);
	polyFree(arg);
	return result;
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
	Poly retval = K2Poly(k);
	freeK(k);
	freeK(ksec);
	freeK(kmic);
	freeK(shifter);
	return retval;
	#endif
}
Poly builtIn_SUB(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY || polySize(arg) == 0){
		goto err;
	}
	Poly *array = unwrapPolyArray(arg);
	Poly poly = array[0];
	size_t size;
	BlackBoard map = mkBlackBoard();
	int64_t i,j;
	for(i = 1;i < polySize(arg);i++){
		Poly tuple = array[i];
		if(polyType(tuple) != ARRAY || polySize(tuple) != 2){
			goto err;
		}
		Poly left = unwrapPolyArray(tuple)[0];
		Poly right = unwrapPolyArray(tuple)[1];
		if(polyType(left) == ARRAY || polySize(left) != 1){
			goto err;
		}
		int64_t index = -1;
		Term term = left.ptr.terms[0];
		for(j = 0;j < termSize(term);j++ ){
			if(termDegree(term,j)){
				if(index >= 0){
					goto err;
				}
				index = j;
			}
		}
		char buff[32];
		size = sprintf(buff,"%ld",index);
		map = insert2BlackBoard(map,mkDefinition(buff,size,polyDup(right)));
	}
	Poly val = polyDup(zeroPoly);
	for(i = 0;i < polySize(poly);i++){
		Term term = poly.ptr.terms[i];
		Poly tmp = K2Poly(term.coefficient);
		for(j = 0;j < termSize(term);j++){
			if(termDegree(term,j)){
				char buff[16];
				size = sprintf(buff,"%ld",j);
				Poly value = findFromBlackBoard(map,buff,size);
				if(isNullPoly(value)){
					Term tmp;
					setTermSize(tmp,j+1);
					termDegreeAllocator(tmp);
					setTermDegree(tmp,j,1);
					copyK(tmp.coefficient,K_1);
					value = term2Poly(tmp);
				}
				#if BOOLEAN
				tmp = _polyMul(tmp,value);
				#else
				int64_t k;
				for(k = 0;k < termDegree(term,j);k++){
					Poly temp = polyMul(tmp,value);
					polyFree(tmp);
					tmp = temp;
				}
				polyFree(value);
				#endif
			}
		}
		val = _polyAdd(val,tmp);
	}
	polyFree(arg);
	freeBlackBoard(map);
	return val;
	err : 
	fprintf(stderr,"That is not how you use \\SUB. \n");
	fprintf(stderr,"You gave me : ");
	polyPrint(arg,K2str,stderr);
	fprintf(stderr,"\n");
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
	//fprintf(stderr,"built-in function \"DIE\" is called.\n");
	//fprintf(stderr,"bye bye.\n");
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
				if(polyCmp(array[i],array[i+1])){
					polyFree(arg);
					return polyDup(zeroPoly);
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
	return K2Poly(K_1);
}

Poly builtIn_LSHIFT(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY){
		#if BOOLEAN
		size_t i;
		for(i = 0;i < polySize(arg);i++){
			Term *term = &arg.ptr.terms[i];
			if(term->sizu <= sizeof(N)*8){
				term->deg.val >>= 1;
			}else{
				size_t j;
				term->deg.ptr[0] >>= 1;
				for(j = 1;j * (sizeof(N)*8) < termSize(*term);j++){
					term->deg.ptr[j - 1] |= (term->deg.ptr[j] & 1) << (sizeof(N) * 8 - 1);
					term->deg.ptr[j] >>= 1;
				}
			}
		}
		return arg;
		#else
		size_t i;
		for(i = 0;i < polySize(arg);i++){
			Term term = arg.ptr.terms[i];
			if(termSize(term) == 0){
				//constant. ignoring.
				continue;
			}
			size_t newSize = termSize(term) - 1;
			Term newTerm;
			setTermSize(newTerm,newSize);
			copyK(newTerm.coefficient,term.coefficient);
			termDegreeAllocator(newTerm);
			size_t j;
			if(termDegree(term,0)){
				fprintf(stderr,"A variable x_0 disappeared in \\LSHIFT, you probably don't want it.\n");
				DIE;
			}
			for(j = 1;j < termSize(term);j++){
				N val = termDegree(term,j);
				if(val){
					setTermDegree(newTerm,j - 1,val);
				}
			}
			termFree(term);
			arg.ptr.terms[i] = newTerm;
		}
		return _polySort(arg);
		#endif
	}else{
		Poly *array = unwrapPolyArray(arg);
		size_t i;
		size_t index = 0;
		for(i = 0;i < polySize(arg);i++){
			if(isNullPoly(array[index] = builtIn_LSHIFT(polyDup(array[i]),blackboard))
				|| isZeroPoly(array[index])){
				polyFree(array[index]);
			}else{
				index++;
			}
		}
		setPolySize(arg,index);
		polyFree(arg);
		return arg;
	}
}
Poly builtIn_RSHIFT(Poly arg,BlackBoard blackboard){
	if(polyType(arg) == POLY){
		size_t i;
		for(i = 0;i < polySize(arg);i++){
			Term term = arg.ptr.terms[i];
			if(termSize(term) == 0){
				//constant. ignoring.
				continue;
			}
			size_t newSize = termSize(term) + 1;
			Term newTerm;
			setTermSize(newTerm,newSize);
			copyK(newTerm.coefficient,term.coefficient);
			termDegreeAllocator(newTerm);
			size_t j;
			for(j = 0;j < termSize(term);j++){
				N val = termDegree(term,j);
				if(val){
					setTermDegree(newTerm,j + 1,val);
				}
			}
			termFree(term);
			arg.ptr.terms[i] = newTerm;
		}
		return _polySort(arg);
	}else{
		Poly *array = unwrapPolyArray(arg);
		size_t i;
		size_t index = 0;
		for(i = 0;i < polySize(arg);i++){
			if(isNullPoly(array[index] = builtIn_RSHIFT(polyDup(array[i]),blackboard))
				|| isZeroPoly(array[index])){
				polyFree(array[index]);
			}else{
				index++;
			}
		}
		setPolySize(arg,index);
		polyFree(arg);
		return arg;
	}
}
Poly builtIn_CAR(Poly arg,BlackBoard blackboard){
	if(polyType(arg) == ARRAY){
		if(polySize(arg) == 0){
			DIE;
		}
		Poly retval = polyDup(unwrapPolyArray(arg)[0]);
		polyFree(arg);
		return retval;
	}
	return arg;
}
Poly builtIn_CDR(Poly arg,BlackBoard blackboard){
	if(polyType(arg) == ARRAY){
		if(polySize(arg) == 1){
			polyFree(arg);
			return polyDup(nullPoly);
		}
		polyFree(unwrapPolyArray(arg)[0]);
		if(polySize(arg) == 2){
			return unwrapPolyArray(arg)[1];
		}
		return mkPolyArray(&unwrapPolyArray(arg)[1],polySize(arg) - 1);
	}
	polyFree(arg);
	return polyDup(nullPoly);
}
Poly builtIn_PPSIZE(Poly arg,BlackBoard blackboard){
	if(isZeroPoly(arg)){
		printf("0");
		return arg;
	}
	printf("%lu",polySize(arg));
	return arg;
}
Poly builtIn_CMP(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY || polySize(arg) != 2){
		fprintf(stderr,"You are using \\CMP wrong.\n");
		DIE;
	}
	Poly *ptr;
	Poly groebner_basis = unwrapPolyArray(arg)[1];
	Poly retval;
	size_t size;
	if(polyType(arg) == ARRAY){
		ptr = unwrapPolyArray(unwrapPolyArray(arg)[0]);
		size = polySize(unwrapPolyArray(arg)[0]);
	}else{
		ptr = &unwrapPolyArray(arg)[0];
		size = 1;
	}
	int i;
	for(i = 0;i < size;i++){
		Poly result = polySim(ptr[i],groebner_basis);
		if(! isZeroPoly(result)){
			polyFree(result);
			retval = polyDup(zeroPoly);
			goto ret;
		}
		polyFree(result);
	}
	ret:
	polyFree(arg);
	retval = polyDup(onePoly);
	return retval;
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
		.name = "CAR",
		.description = "Return first element of an array.",
		.funcptr = builtIn_CAR
	},
	{
		.name = "CDR",
		.description = "Return given array without first element of it.",
		.funcptr = builtIn_CDR
	},
	{
		.name = "CLOCK",
		.description = "Call clock() function.",
		.funcptr = builtIn_CLOCK
	},
	{
		.name = "CMP",
		.description = "Return 1 if 1st argument is inside ideal given by 2nd groebner basis",
		.funcptr = builtIn_CMP
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
		.name = "LSHIFT",
		.description = "Shift subscript by one.",
		.funcptr = builtIn_LSHIFT
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
		.name = "PPP",
		.description = "Print given polynomials with PoriBori format.",
		.funcptr = builtIn_PPP
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
		.name = "PPSIZE",
		.description = "Print size of polynomials.",
		.funcptr = builtIn_PPSIZE
	},
	{
		.name = "RED",
		.description = "Reduce polynomial",
		.funcptr = builtIn_RED
	},
	{
		.name = "RSHIFT",
		.description = "Shift subscript by one.",
		.funcptr = builtIn_RSHIFT
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

