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
size_t SUBSHIFT = 0;
void *LOADED_FUNCTION_PTR = NULL;
size_t LOADED_FUNCTION_INPUT_SIZE = 0;
size_t LOADED_FUNCTION_OUTPUT_SIZE = 0;

const char PRE_INCLUDE[] = 
	"LEX = 0 \\\\ RLEX = 1 \\\\ PLEX = 2\\\\ PRLEX = 3\\\\"
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

#include <time.h>
int main(int argc,char *argv[]){
	initConst();
	OUTFILE = stdout;
	FILE *infile = stdin;
	const char optstr[] = "s:l:f:";
	opterr = 0;
	int c;
	void *dlopen_handle = NULL;
	char *functionName = NULL;
	while((c = getopt(argc,argv,optstr)) != -1){
		switch(c){
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
		#if !DEBUG
			poly = parser(infile,&blackboard);
			cmdNumber++;
		#else 
			poly = parser(infile,&blackboard);
			cmdNumber++;
			if(polyType(poly) != ARRAY){
				size_t size = 0;
				size_t i;
				for(i = 0;i < polySize(poly);i++){
					#if BOOLEAN
					size += termSize(poly.ptr.terms[i])/sizeof(N) + 1;
					#elif RATIONAL
					size += sizeof(N)*termSize(poly.ptr.terms[i]);
					#else
					#error Nope.
					#endif
					size += sizeof(Term);
				}
				fprintf(stderr,"%lu => sizeof({.size == %lu,.type == %d}) == %lu\n",cmdNumber,polySize(poly),polyType(poly),size);
			}
		#endif
		if(blackboardSize == blackboard.size){
			//parser did not define anything.
			polyFree(poly);
		}
	}
	return 0;
}

