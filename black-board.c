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

const Definition nullDefinition = {
	.bytes = {0},
	.poly =	{
		.size = -1,
		.ptr.items = NULL
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
		polyPrint(blackboard.array[i].poly,K2str,fp);
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

