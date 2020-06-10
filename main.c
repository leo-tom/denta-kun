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
#include <dlfcn.h>

FILE *OUTFILE = NULL;
int64_t SUBSHIFT = 0;
int MONOMIAL_ORDER=0;
int (*_cmpTerm)(const Term *v1,const Term *v2) = _polycmp_LEX;
void *LOADED_FUNCTION_PTR = NULL;
size_t LOADED_FUNCTION_INPUT_SIZE = 0;
size_t LOADED_FUNCTION_OUTPUT_SIZE = 0;
Poly onePoly;

const char PRE_INCLUDE[] = 
	"NULL = \\\\ null = \\\\ X = 0 \\\\"
	#if BOOLEAN
	"BCA_PERIODIC = 1 \\\\ BCA_REFLECTIVE = 0 \\\\ BCA_FIXED = 0 \\\\ BCA_FIXED_VALUE = 0 \\\\"
	"BCA_INITIAL_STATE = (0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0) \\\\"
	#endif
	;



BlackBoard readPreInclude(BlackBoard blackboard){
	int fds[2];
	if(pipe(fds)){
		DIE;
	}
	int rfd = fds[0];
	int wfd = fds[1];
	write(wfd,PRE_INCLUDE,strlen(PRE_INCLUDE));
	char *cmd = getenv("DENTAKUN_CMD");
	if(cmd != NULL){
		write(wfd,cmd,strlen(cmd));
	}
	close(wfd);
	FILE *fp = fdopen(rfd,"r");
	while(!feof(fp)){
		parser(fp,&blackboard);
	}
	return blackboard;
}

size_t polySizeInByte(Poly p){
	size_t retval = 0;
	size_t i;
	if(polyType(p) == ARRAY){
		Poly *array = unwrapPolyArray(p);
		for(i = 0;i < polySize(p);i++){
			retval += polySizeInByte(array[i]);
		}
	}else{
		for(i = 0;i < polySize(p);i++){
			#if BOOLEAN
				retval += termSize(p.ptr.terms[i])/sizeof(N) + 1;
			#elif RATIONAL
				retval += sizeof(N)*termSize(p.ptr.terms[i]);
			#else
				#error Nope.
			#endif
			retval += sizeof(Term);
		}
	}
	return retval;
}

#include <time.h>
int main(int argc,char *argv[]){
	initConst();
	OUTFILE = stdout;
	FILE *infile = stdin;
	const char optstr[] = "s:l:f:i:o:m:";
	opterr = 0;
	int c;
	void *dlopen_handle = NULL;
	char *functionName = NULL;
	while((c = getopt(argc,argv,optstr)) != -1){
		switch(c){
			case 'm':{
				if(!strcmp(optarg,"LEX") ){
					MONOMIAL_ORDER = MONOMIAL_ORDER_LEX;
					_cmpTerm = _polycmp_LEX;
				}else if(!strcmp(optarg,"RLEX") ){
					MONOMIAL_ORDER = MONOMIAL_ORDER_RLEX;
					_cmpTerm = _polycmp_RLEX;
				}else if(!strcmp(optarg,"PLEX") ){
					MONOMIAL_ORDER = MONOMIAL_ORDER_PLEX;
					_cmpTerm = _polycmp_PLEX;
				}else if(!strcmp(optarg,"PRLEX") ){
					MONOMIAL_ORDER = MONOMIAL_ORDER_PRLEX;
					_cmpTerm = _polycmp_PRLEX;
				}else{
					fprintf(stderr,"Unknown monomial order \"%s\"\n",optarg);
					DIE;
				}
			}break;
			case 's':
			{
				SUBSHIFT = atoi(optarg);
			}break;
			case 'l':
			{
				dlopen_handle = dlopen(optarg, RTLD_LAZY);
				if(dlopen_handle == NULL){
					fprintf(stderr,"dlopen failed : %s\n",dlerror());
					DIE;
				}
			}break;
			case 'f':
			{
				LOADED_FUNCTION_PTR = dlsym(dlopen_handle,optarg);
				if(LOADED_FUNCTION_PTR == NULL){
					fprintf(stderr,"dlsym failed : %s\n",dlerror());
					DIE;
				}
				functionName = strdup(optarg);
			}break;
			case 'i':
			{
				LOADED_FUNCTION_INPUT_SIZE = atoi(optarg);
			}break;
			case 'o':
			{
				LOADED_FUNCTION_OUTPUT_SIZE = atoi(optarg);
			}break;
			default:{
				fprintf(stderr,"Unknow option : %c\n",c);
				DIE;
			}
		}
	}
	copyK(zeroPoly.ptr.terms[0].coefficient,K_0);
	onePoly = K2Poly(K_1);
	BlackBoard blackboard = readPreInclude(mkBlackBoard());
	if(LOADED_FUNCTION_PTR != NULL){
		if(LOADED_FUNCTION_INPUT_SIZE == 0 || LOADED_FUNCTION_OUTPUT_SIZE == 0){
			fprintf(stderr,"Use -i option and -o option to specify input size and output size.\n");
			DIE;
		}
		Poly poly = cfunc2Poly(LOADED_FUNCTION_INPUT_SIZE,LOADED_FUNCTION_OUTPUT_SIZE,blackboard);
		blackboard = insert2BlackBoard(blackboard
				,mkDefinition(functionName,strlen(functionName),poly));
	}
	uint64_t cmdNumber = 0;
	while(!feof(infile)){
		Poly poly;
		size_t blackboardSize = blackboard.size;
		#if DEBUG == 0
			poly = parser(infile,&blackboard);
			cmdNumber++;
		#else 
			poly = parser(infile,&blackboard);
			cmdNumber++;
			polyPrint(poly,K2str,stderr);
			fprintf(stderr," => %lu\n",polySizeInByte(poly));
		#endif
		if(blackboardSize == blackboard.size){
			//parser did not define anything.
			polyFree(poly);
		}
	}
	return 0;
}

