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

int getNextChar(FILE *stream){
	int c;
	while((c = fgetc(stream))!=EOF){
		switch(c){
			case ' ':
			case '\t':
			case '\n':
				break;
			default :
				return c;
		}
	}
	return c;
}

enum NodeType {
	Command,
	Variable,
	Number,
	Block,
};
typedef struct _Node{
	enum NodeType type;
	char str[64];
	struct _Node *next;
}Node;

#define append(head,butt,nodeType,data) do{ \
	Node *newNodeDesu = malloc(sizeof(Node)); \
	newNodeDesu->type = nodeType; \
	strcpy(newNodeDesu->str,data); \
	newNodeDesu->next = NULL; \
	if(butt == NULL){ \
		head = butt = newNodeDesu; \
	}else{ \
		butt->next = newNodeDesu; \
		butt = newNodeDesu;\
	} \
}while(0)

Node * __parser(FILE *stream){
	int c;
	
	Node *head,*butt;
	head = butt = NULL;
	
	const enum NodeType command = Command;
	const enum NodeType variable = Variable;
	const enum NodeType number = Number;
	
	
	while((c = getNextChar(stream))!= EOF){
		switch(c){
			case '+':
			case '-':
			case '^':
			case '_':
			{
				char buff[2];
				buff[0] = c; buff[1] = 0;
				append(head,butt,command,buff);
			}
			break;
			case '\\':
			{
				if((c = getNextChar(stream)) == '\\'){
					return head;
				}
				ungetc(c,stream);
				char _buff[256];
				char *buff = _buff;
				while(isalpha(c = getNextChar(stream)) ){
					*buff++ = c;
				}
				ungetc(c,stream);
				append(head,butt,variable,_buff);
			}
			case '{':
			{
				Node *new = malloc(sizeof(Node));
				new->type = Block;
				new->next = NULL;
				void *insideBlock = __parser(stream);
				memcpy(new->str,&insideBlock,sizeof(void *));
				if(butt == NULL){
					head = butt = new;
				}else{
					butt->next = new;
					butt = new;
				}
			}
				break;
			case '}':
				return head;
			default :
				if(isdigit(c) || c == '-'){
					char _buff[256] = {0};
					char *buff = _buff;
					*buff++ = c;
					while(isdigit(c = getNextChar(stream)) || c == '.' ){
						*buff++ = c;
					}
					ungetc(c,stream);
					*buff = 0;
					append(head,butt,number,_buff);
				}else if(isalpha(c)){
					char _buff[256]  = {0};
					char *buff = _buff;
					*buff++ = c;
					while(isalpha(c = getNextChar(stream)) ){
						*buff++ = c;
					}
					ungetc(c,stream);
					*buff = 0;
					append(head,butt,variable,_buff);
				}else{
					fprintf(stderr,"I dont know how to parse %c[%x]\n",c,c);
					DIE;
				}
			break;	
		}
	}
	return head;
}

Poly _parser(Node *n){
	Node *now = n;
	Poly retval = {
		.size = 1;
		.items = NULL;
	};
	
	size_t heapSize = 16;
	
	Item item = {
		.size = 0;
		.coefficient = 1;
		.degrees = calloc(sizeof(N),heapSize);
	};
	
	while(now){
		switch(now->type){
			case Command:{
				if(!strcmp(now->str,"+") || !strcmp(now->str,"-")){
					Poly tmp = appendItem2Poly(retval,item);
					polyFree(retval);
					retval = tmp;
					free(item.indexes);item.indexes = calloc(sizeof(N),heapSize);
					free(item.degrees);item.indexes = calloc(sizeof(N),heapSize);
					heapSize = 16;
					item.size = 0;
					if(!strcmp(now->str,"-")){
						item.coefficient *= -1;
					}else{
						item.coefficient = 1;
					}
					//ok
				}else if(!strcmp(now->str,"times") || !strcmp(now->str,"cdot")){
					now = now->next
					if(now->type == Number){
						item.coefficient *= atof(now->str);
					}else if(now->type == Block){
						Poly tmp = _parser(*((Node **)&now->str));
						item.coefficient *= poly2Double(tmp);
						polyFree(tmp);
					}else{
						DIE;
					}
				}else if(!strcmp(now->str,"/")){
					now = now->next
					if(now->type == Number){
						item.coefficient /= atof(now->str);
					}else if(now->type == Block){
						Poly tmp = _parser(*((Node **)&now->str));
						item.coefficient /= poly2Double(tmp);
						polyFree(tmp);
					}else{
						DIE;
					}
				}
			}break;
			case Number:{
				item.coefficient *= atof(now->str); 
			}break;
			case Block:{
				Poly tmp = _parser(*((Node **)&now->str));
				item.coefficient *= poly2Double(tmp);
				polyFree(tmp);
			}break;
			case Variable:{
				/*Ok. I think...*/
				if(!strcmp(now->str,"x")){
					Node *prev = now;
					now = now->next;
					int counter = 0;
					int index = -1;
					N degrees = -1;
					for(counter = 0;counter < 2;counter++){
						if(now == NULL){
							break;
						}else if(!strcmp(now->str,"_") || !strcmp(now->str,"^"){
							now = now->next;
							N n;
							switch(now->type){
								case Block:{
									Poly tmp = _parser(*((Node **)&now->str));
									n = (N)(poly2Double(tmp) + 0.5);
								}break;
								case Number:{
									n = atol(now->next->str);
								}break;
								default : {
									fprintf(stderr,"After \'%s\', a number or block must follow",now->str);
									DIE;
								}break;
							}
							
							if(!strcmp(now->str,"_")){
								index = n;
							}else{
								degrees = n;
							}
						}else{
							now = prev;
							break;
						}
						prev = now;
						now = now->next;
					}
					if(index >= size && degrees > 0){
						N *newDegrees = calloc(sizeof(N),index + 1);
						memcpy(newDegrees,item.degrees,size);
						free(item.degrees);
						item.degrees = newDegrees;
						item.size = index + 1;
					}
					if(degrees > 0){
						item.degrees[index] += degrees;
					}
				}else{
					fprintf(stderr,"Currently dentakun cannot accept variable name besides \'x\'\n");
					DIE;
				}
			}
			now = now->next;
		}
	}
	
	if(item.size){
		Poly tmp = appendItem2Poly(retval,item);
		polyFree(retval);
		retval = tmp;
		free(item.indexes);
		free(item.degrees);
	}
	return retval;
}
Poly parser(FILE *stream){
	int c;
	while((c = fgetc(stream))!=EOF){
		if(c == '%'){
			while((c = fgetc(stream))!= EOF && c != '\n')
				;
		}else{
			return _parse(__parse(stream));
		}
	}
	return nullPoly;
}

void _print_parsed_tex(Node *n,FILE *fp){
	if(n == NULL){
		return;
	}
	Node *now = n;
	while(now){
		switch(now->type){
			case Number : {
				fprintf(fp," %f ",atof(now->str));
			}break;
			case Variable : {
				fprintf(fp," %s ",now->str);
			}break;
			case Command : {
				switch(now->str[0]){
					case '+':
					case '-':
					case '^':
					case '_':
						fprintf(fp," %c ",now->str[0]);
					break;
					default :
						fprintf(fp," \\%s ", now->str);
					break;
				}
			}break;
			case Block : {
				Node *ptr;
				memcpy(&ptr,now->str,8);
				fprintf(fp,"{");
				_print_parsed_tex(ptr,fp);
				fprintf(fp,"}");
			}
			break;
		}
		now = now->next;
	}
}	

int main(int argc,char *argv[]){
	FILE *outfile = stdout;
	Node *n = _parser(stdin);
	_print_parsed_tex(n,outfile);
	fprintf(outfile,"\n");
	return 0;
}

