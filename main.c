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

FILE *OUTFILE = NULL;

const char PRE_INCLUDE[] = 
	"LEX = 0 \\\\ RLEX = 1 \\\\ PLEX = 2\\\\"
	#if BOOLEAN
	"BCA_INITIAL_STATE = 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
	"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 \\\\"
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
#include <time.h>
int main(int argc,char *argv[]){
	OUTFILE = stdout;
	FILE *infile = stdin;
	initConst();
	copyK(zeroPoly.ptr.items[0].coefficient,K_0);
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
	return 0;
}

