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

FILE *OUTFILE = NULL;

const char PRE_INCLUDE[] = "LEX = 0 \\\\ RLEX = 1 \\\\ PLEX = 2\\\\";

BlackBoard readPreInclude(BlackBoard blackboard){
	int fds[2];
	if(pipe(fds)){
		DIE;
	}
	int rfd = fds[0];
	int wfd = fds[1];
	write(wfd,PRE_INCLUDE,strlen(PRE_INCLUDE));
	close(wfd);
	FILE *fp = fdopen(rfd,"r");
	Definition definition = parser(fp,blackboard);
	while(!feof(fp)){
		if(strlen(getNameFromDefinition(&definition)) == 0){
			DIE;
		}
		blackboard = insert2BlackBoard(blackboard,definition);
		definition = parser(fp,blackboard);
	}
	return sortBlackBoard(blackboard);
}

int main(int argc,char *argv[]){
	OUTFILE = stdout;
	FILE *infile = stdin;
	initConst();
	BlackBoard blackboard = readPreInclude(mkBlackBoard());
	Definition definition = parser(infile,blackboard);
	unsigned int anonIndex = 0;
	char buff[128];
	while(!feof(infile)){// while definition != nullDefinition
		if(strlen(getNameFromDefinition(&definition)) == 0){
			sprintf(buff,"$%u",anonIndex++);
			definition = mkDefinition(buff,strlen(buff),definition.poly);
		}
		blackboard = insert2BlackBoard(blackboard,definition);
		definition = parser(infile,blackboard);
	}
	blackboard = sortBlackBoard(blackboard);
	
	return 0;
}

