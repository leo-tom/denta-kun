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
	Say
};

#define NODE_STR_SIZE (128)
typedef struct _Node{
	enum NodeType type;
	char str[NODE_STR_SIZE];
	struct _Node *next;
}Node;

#define append(head,butt,nodeType,data) do{ \
	Node *newNodeDesu = malloc(sizeof(Node)); \
	newNodeDesu->type = nodeType; \
	memcpy(newNodeDesu->str,data,NODE_STR_SIZE); \
	newNodeDesu->next = NULL; \
	if(butt == NULL){ \
		head = butt = newNodeDesu; \
	}else{ \
		butt->next = newNodeDesu; \
		butt = newNodeDesu;\
	} \
}while(0)

Poly _parser(Node *head,Node *tail,BlackBoard *blackboard);

Node * __parser(FILE *stream){
	int c;
	
	Node *head,*butt;
	head = butt = NULL;
	
	const enum NodeType command = Command;
	const enum NodeType variable = Variable;
	const enum NodeType number = Number;
	
	
	while((c = getNextChar(stream))!= EOF){
		switch(c){
			case '%':{
				if((c = fgetc(stream)) == ':'){
					while((c = fgetc(stream)) != '\n' && c != EOF){
						if(c == '\\'){
							c = fgetc(stream);
							switch(c){
								case '\\':{
									c = '\\';
								}break;
								case 'r':{
									c = '\r';
								}break;
								case 'n':{
									c = '\n';
								}break;
								case 't':{
									c = '\t';
								}break;
								default:{
									fprintf(stderr,"Unknown special char \'\\%c\'[%x]\n",c,c);
									DIE;
								}break;
							}
						}
						fputc(c,OUTFILE);
					}
				}else{
					ungetc(c,stream);
					while((c = fgetc(stream))!= '\n' ){
						;
					}
				}
				break;
			}
			case '=':
			case '+':
			case '^':
			case '_':
			case ',':
			{
				char buff[NODE_STR_SIZE];
				buff[0] = c; buff[1] = 0;
				append(head,butt,command,buff);
				break;
			}
			case '-':
			{
				char buff[NODE_STR_SIZE] = {0};
				buff[0] = '+'; buff[1] = 0;
				append(head,butt,command,buff);
				strcpy(buff,"-1");
				append(head,butt,Number,buff);
				break;
			}
			case '\\':
			{
				if((c = getNextChar(stream)) == '\\'){
					return head;
				}
				ungetc(c,stream);
				char _buff[NODE_STR_SIZE] = {0};
				char *buff = _buff;
				while(isalpha(c = fgetc(stream)) ){
					*buff++ = c;
				}
				*buff = 0;
				ungetc(c,stream);
				append(head,butt,command,_buff);
				break;
			}
			case '{':
			case '(':
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
				break;
			}
			case '}':
			case ')':
				{
					return head;
				}
			default :
			{
				if(isdigit(c) || c == '-'){
					char _buff[NODE_STR_SIZE] = {0};
					char *buff = _buff;
					*buff++ = c;
					while(isdigit(c = fgetc(stream)) || c == '.' ){
						*buff++ = c;
					}
					ungetc(c,stream);
					*buff = 0;
					append(head,butt,number,_buff);
				}else if(isalpha(c)){
					char _buff[NODE_STR_SIZE] = {0};
					char *buff = _buff;
					*buff++ = c;
					while(isalpha(c = fgetc(stream)) || isdigit(c)){
						*buff++ = c;
					}
					ungetc(c,stream);
					*buff = 0;
					append(head,butt,variable,_buff);
				}else{
					fprintf(stderr,"Unknown char\'%c\'[%x]\n",c,c);
					DIE;
				}
				break;
			}	
		}
	}
	return head;
}
#if DEBUG >= 1
void _print_parsed_tex(Node *n,FILE *fp){
	if(n == NULL){
		return;
	}
	Node *now = n;
	while(now){
		switch(now->type){
			case Number : {
				fprintf(fp," %s ",now->str);
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
			case Say : {
				fprintf(fp," \"%s\" ",now->str);
			}
			break;
		}
		now = now->next;
	}
}
#endif

Node * variableName(Node *node,char *str){
	enum NodeType lastType = Number;
	while(node != NULL && (node->type == Number || node->type == Variable || (node->type == Command && !strcmp(node->str,"_")) )){
		if(lastType == Variable && node->type == Variable){
			break;
		}
		str = stpcpy(str,node->str);
		lastType = node->type;
		node = node->next;
	}
	return node;
}

//Lisp
Poly executeSpecialFunction(Node *head,Node **next,BlackBoard *blackboard){
	char *name = head->str;
	if(!strcmp(name,"IF")){
		if(head->next == NULL || head->next->next == NULL || head->next->next->next == NULL){
			fprintf(stderr,"IF function takes three arguments.\n");
			DIE;
		}
		Poly state = _parser(head->next,head->next->next,blackboard);
		Poly retval = nullPoly;
		if(isNullPoly(state) || isZeroPoly(state)){
			retval = _parser(head->next->next->next,head->next->next->next->next,blackboard);
		}else{
			retval = _parser(head->next->next,head->next->next->next,blackboard);
		}
		*next = head->next->next->next->next;
		polyFree(state);
		return retval;
	}else if(!strcmp(name,"FOREACH")){
		if(head->next == NULL || head->next->next == NULL ){
			fprintf(stderr,"FOREACH function takes two arguments.\n");
			DIE;
		}
		Poly _array = _parser(head->next,head->next->next,blackboard);
		Poly *array;
		size_t arraySize;
		if(polyType(_array) == ARRAY){
			array = unwrapPolyArray(_array);
			arraySize = polySize(_array);
		}else{
			array = &_array;
			arraySize = 1;
		}
		size_t i;
		Poly retval = nullPoly;
		for(i = 0;i < arraySize;i++){
			mut Poly tmp = findFromBlackBoard(*blackboard,"X",strlen("X"));
			unmut Poly X = array[i];
			*blackboard = insert2BlackBoard(*blackboard,mkDefinition("X",strlen("X"),polyDup(X)));
			polyFree(retval);
			retval = _parser(head->next->next,head->next->next->next,blackboard);
			*blackboard = insert2BlackBoard(*blackboard,mkDefinition("X",strlen("X"),tmp));
		}
		*next = head->next->next->next;
		*blackboard = insert2BlackBoard(*blackboard,mkDefinition("X",strlen("X"),polyDup(zeroPoly)));
		polyFree(_array);
		return retval;
	}else if(!strcmp(name,"WHILE")){
		if(head->next == NULL || head->next->next == NULL ){
			fprintf(stderr,"WHILE function takes two arguments.\n");
			DIE;
		}
		Poly retval = nullPoly;
		Poly state = _parser(head->next,head->next->next,blackboard);
		while(!isNullPoly(state) && !isZeroPoly(state)){
			polyFree(retval);
			polyFree(state);
			retval = _parser(head->next->next,head->next->next->next,blackboard);
			state = _parser(head->next,head->next->next,blackboard);
		}
		*next = head->next->next->next;
		polyFree(state);
		return retval;
	}else if(!strcmp(name,"frac") || !strcmp(name,"FRAC")){
		if(head->next == NULL || head->next->next == NULL ){
			fprintf(stderr,"FRAC function takes two arguments.\n");
			DIE;
		}
		Node *nBunsi = head->next;
		Node *nBunbo = head->next->next;
		*next = head->next->next->next;
		Poly bunbo = _parser(nBunbo,nBunbo->next,blackboard);
		Poly bunsi = _parser(nBunsi,nBunsi->next,blackboard);
		if(polyType(bunbo) == ARRAY || polyType(bunsi) == ARRAY){
			fprintf(stderr,"Invalid argument for frac.\n");
			DIE;
		}else if(isZeroPoly(bunsi)){
			fprintf(stderr,"Division by zero.\n");
			DIE;
		}else if(isNullPoly(bunsi) || isNullPoly(bunbo)){
			polyFree(bunsi);polyFree(bunbo);
			return nullPoly;
		}
		Poly array = polyDiv(bunbo,bunsi);
		Poly retval = polyDup(unwrapPolyArray(array)[0]);
		polyFree(array);
		polyFree(bunbo);
		polyFree(bunsi);
		return retval;
	}
	DIE;
}

int isSpecialFunction(const char * const name){
	if(!strcmp(name,"IF")
	|| !strcmp(name,"FOREACH")
	|| !strcmp(name,"WHILE")
	|| !strcmp(name,"frac")
	|| !strcmp(name,"FRAC")
		){
		return 1;
	}
	return 0;
}

Poly _parser(Node *head,Node *tail,BlackBoard *blackboard){
	if(head == NULL || head == tail){
		return nullPoly;
	}
	Node *now = head;
	while(now != tail && now!= NULL){
		if(now->type == Command && !strcmp(now->str,"=")){
			char buff[256];
			if(variableName(head,buff) != now){
				fprintf(stderr,"Left hand side must be name of variable\n");
				DIE;
			}
			Poly retval = _parser(now->next,tail,blackboard);
			*blackboard = insert2BlackBoard(*blackboard,mkDefinition(buff,strlen(buff),polyDup(retval)));
			return retval;
		}
		now = now->next;
	}
	now = head;
	size_t capacity = 8;
	Poly *ptr = malloc(sizeof(Poly) * capacity);
	size_t s = 0;
	while(now!=tail && now!=NULL){
		if(now->type == Command && !strcmp(now->str,",")){
			if(s >= capacity){
				capacity *= 2;
				ptr = realloc(ptr,sizeof(Poly) * capacity);
			}
			ptr[s++] = _parser(head,now,blackboard);
			head = now->next;
		}
		now = now->next;
	}
	if(s > 0){
		if(s + 1 >= capacity){
			capacity = s + 1;
			ptr = realloc(ptr,sizeof(Poly) * capacity);
		}
		ptr[s++] = _parser(head,tail,blackboard);
		if(isNullPoly(ptr[s-1])){
			s--;
		}
		return mkPolyArray(ptr,s);
	}else{
		free(ptr);
	}
	now = head;
	while(now != tail && now != NULL){
		switch(now->type){
			case Command:{
				if(!strcmp(now->str,"+")){
					Poly pLeft = _parser(head,now,blackboard);
					Poly pRight = _parser(now->next,tail,blackboard);
					Poly retval;
					if(head != now){ 
						retval = polyAdd(pLeft,pRight);
					}else{
						return pRight;
					}
					polyFree(pLeft);
					polyFree(pRight);
					return retval;
				}
				break;
			}
			default:{
				break;
			}
		}
		now = now->next;
	}
	
	now = head;
	while(now != tail && now){
		switch(now->type){
			case Command:{
				if(!strcmp(now->str,"times") || !strcmp(now->str,"cdot")){
					Poly pLeft = _parser(head,now,blackboard);
					Poly pRight = _parser(now->next,tail,blackboard);
					Poly retval = polyMul(pLeft,pRight);
					polyFree(pLeft);
					polyFree(pRight);
					return retval;
				}
				break;
			}
			default:{
				break;
			}
		}
		now = now->next;
	}
	Poly retval = K2Poly(K_1,LEX);
	
	now = head;
	while(now != tail && now){
		switch(now->type){
			case Command:{
				const char * const str = now->str;
				Poly p;
				if(isSpecialFunction(str)){
					Node *next;
					p = executeSpecialFunction(now,&next,blackboard);
					now = next;
				}else{
					now = now->next;
					if(now == NULL){
						p = callBuiltInFunc(str,nullPoly,*blackboard);
					}else{
						p = callBuiltInFunc(str,_parser(*((Node **)&now->str),NULL,blackboard),*blackboard);
						now = now->next;
					}
				}
				if(polyType(p) == ARRAY){
					polyFree(retval);
					return p;
				}else if(isNullPoly(p)){
					return p;
				}
				Poly tmp = polyMul(retval,p);
				polyFree(p);
				polyFree(retval);
				retval = tmp;
				break;
			}
			case Block:{
				Poly p = _parser(*((Node **)&now->str),NULL,blackboard);
				if(now->next == NULL && head == now){
					polyFree(retval);
					return p;
				}else if(polyType(p) == ARRAY){
					polyFree(retval);
					return p;
				}
				Poly tmp = polyMul(retval,p);
				polyFree(p);
				polyFree(retval);
				retval = tmp;
				now = now->next;
				break;
			}
			case Number:{
				K val;
				str2K(val,now->str);
				if(!cmpK(val,K_0)){
					polyFree(retval);
					return polyDup(zeroPoly);
				}
				int i = 0;
				for(i = 0;i < polySize(retval);i++){
					mulK(retval.ptr.terms[i].coefficient,retval.ptr.terms[i].coefficient,val);
				}
				now = now->next;
				break;
			}
			case Variable:{
				if(!strcmp(now->str,"x")){
					now = now->next;
					int counter = 0;
					N index = 0;
					N degrees = 1;
					for(counter = 0;counter < 2;counter++){
						if(now == NULL){
							break;
						}else if(!strcmp(now->str,"_") || !strcmp(now->str,"^")){
							int subscriptDesuka;
							if(!strcmp(now->str,"_")){
								subscriptDesuka = 1;
							}else{
								subscriptDesuka = 0;
							}
							now = now->next;
							uint64_t n;
							switch(now->type){
								case Block:{
									Node *inside_block = *((Node **)&now->str);
									if(inside_block->type == Number && inside_block->next == NULL){
										n = atoll(inside_block->str);
										break;
									}else{
										fprintf(stderr,"Subscript must be just a number.\n");
										DIE;
									}
								}break;
								case Number:{
									n = (N) atoll(now->str);
								}break;
								default : {
									fprintf(stderr,"After \'%s\', a number must follow",now->str);
									DIE;
								}break;
							}
							if(subscriptDesuka){
								index = n;
							}else{
								degrees = n;
							}
							now = now->next;
						}else{
							break;
						}
					}
					Term term;
					setTermSize(term,(index + 1));
					termDegreeAllocator(term);
					#if BOOLEAN
					if(degrees > 1){
						degrees = 1;
					}
					#endif
					setTermDegree(term,index,degrees);
					copyK(term.coefficient,K_1);
					Poly mulDis = term2Poly(term);
					Poly tmp = polyMul(retval,mulDis);
					polyFree(retval);
					polyFree(mulDis);
					retval = tmp;
				}else{
					char buff[256];
					now = variableName(now,buff);
					Poly mulDis = findFromBlackBoard(*blackboard,buff,strlen(buff));
					if(isNullPoly(mulDis) ) {
						polyFree(retval);
						return mulDis;
					}
					if(polyType(mulDis) == ARRAY){
						polyFree(retval);
						return mulDis;
					}
					if(polyType(mulDis) != polyType(retval)){
						Poly tmp = polySort(mulDis,polyType(retval));
						polyFree(mulDis);
						mulDis = tmp;
					}
					Poly tmp = polyMul(retval,mulDis); polyFree(retval); polyFree(mulDis);
					retval = tmp;
				}
				break;
			}
			case Say:{
				fprintf(OUTFILE,"%s",now->str);
				now = now->next;
				break;
			}
			default :{
				fprintf(stderr,"%s\n",now->str);DIE;
			}
		}
	}
	return retval;
}

void freeNodes(Node *node){
	while(node){
		Node *next = node->next;
		if(node->type == Block){
			freeNodes(*((Node **)&node->str));
		}
		free(node);
		node = next;
	}
}

Poly parser(FILE *stream,BlackBoard *blackboard){
	int c;
	while((c = fgetc(stream))!=EOF){
		ungetc(c,stream);
		Node *nodes = __parser(stream);
		if(nodes == NULL){
			return nullPoly;
		}
		#if DEBUG >= 2
			fprintf(stderr,"Input : ");
			_print_parsed_tex(nodes,stderr);
			fprintf(stderr,"\n");
			
			fprintf(stderr,"Parsed as : ");
		#endif
		Poly retval = _parser(nodes,NULL,blackboard);
		freeNodes(nodes);
		#if DEBUG >= 2
			polyPrint(retval,K2str,stderr);
			fprintf(stderr,"\n");
		#endif
		return retval;
	}
	return nullPoly;
}

Poly instantParser(char *code,BlackBoard *blackboard){
	int p[2];
	pipe(p);
	write(p[1],code,strlen(code));
	close(p[1]);
	FILE *fp = fdopen(p[0],"r");
	Poly retval = parser(fp,blackboard);
	fclose(fp);
	return retval;
}


