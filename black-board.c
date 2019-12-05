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

const Definition nullDefinition = {
	.bytes = {0},
	.poly =	{
		.size = -1,
		.items = NULL
	},
};

int isNullDefinition(unmut Definition definition){
	return isNullPoly(definition.poly) &&  
		!memcmp(nullDefinition.bytes,definition.bytes,DEFINITION_BYTES_SIZE);
}

BlackBoard mkBlackBoard(){
	BlackBoard retval;
	retval.capacity = 8;
	retval.array = malloc(sizeof(Definition)*retval.capacity);
	retval.size = 0;
	return retval;
}

void printBlackBoard(BlackBoard blackboard,FILE *fp){
	size_t i;
	for(i = 0;i < blackboard.size;i++){
		fprintf(fp,"%s = ",getNameFromDefinition(&blackboard.array[i]));
		polyPrint(blackboard.array[i].poly,fp);
		fprintf(fp," \\\n");
	}
}

const char * getNameFromDefinition(unmut const Definition *def){
	if(def->bytes[0] & 0x80){
		return (const char *) (*((char **) (&def->bytes[1])));
	}else{
		return (const char *)def->bytes;
	}
}

void freeDefinition(mut Definition def){
	if(def.bytes[0] & 0x80){
		free(*((char **) &(def.bytes[1])));
	}
}

void freeBlackBoard(mut BlackBoard blackboard){
	size_t i;
	Definition *ptr = blackboard.array;
	for(i = 0;i < blackboard.size;i++){
		freeDefinition(*ptr++);
	}
	free(blackboard.array);
}

Definition mkDefinition(const char *name,size_t nameSize,mut Poly poly){
	Definition retval;
	if(nameSize + 1 < DEFINITION_BYTES_SIZE){
		if(name[0] & 0x80){
			fprintf(stderr,"variable name must be made of ascii chars\n");
			DIE;
		}
		strncpy((char *)retval.bytes,name,DEFINITION_BYTES_SIZE);
	}else{
		char *ptr = malloc(nameSize + 1);
		memcpy(&retval.bytes[1],&ptr,sizeof(char *));
		strcpy(ptr,name);
		retval.bytes[0] = 0xff;
	}
	retval.poly = poly;
	return retval;
}

int cmpDefinition(unmut const Definition *v1,unmut const Definition *v2){
	return strcmp(getNameFromDefinition(v1),getNameFromDefinition(v2));
}

BlackBoard sortBlackBoard(mut BlackBoard blackboard){
	qsort(blackboard.array,blackboard.size,sizeof(Definition)
		,(int (*)(const void *, const void *))cmpDefinition);
	return blackboard;
}

BlackBoard insert2BlackBoard(mut BlackBoard blackboard,mut Definition def){
	if(blackboard.size >= blackboard.capacity){
		blackboard.capacity *= 2;
		blackboard.array = realloc(blackboard.array,sizeof(Definition)*blackboard.capacity);
	}
	blackboard.array[blackboard.size++] = def;
	return blackboard;
}

Poly findFromBlackBoard(unmut BlackBoard blackboard,const char *name,size_t nameSize){
	Definition tmp = mkDefinition(name,nameSize,nullPoly);
	Definition *ptr = bsearch(&tmp,blackboard.array,blackboard.size,sizeof(Definition)
						,(int (*)(const void *, const void *))cmpDefinition);
	freeDefinition(tmp);
	return ptr == NULL ? nullPoly : polyDup(ptr->poly); 
}

