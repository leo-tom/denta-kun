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

extern Item _copyItem(unmut Item item);

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
	memcpy(newNodeDesu->str,data,64); \
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
			case '%':{
				while((c = fgetc(stream))!= '\n' ){
					;
				}
				break;
			}
			case '=':
			case '+':
			case '^':
			case '_':
			case ',':
			{
				char buff[2];
				buff[0] = c; buff[1] = 0;
				append(head,butt,command,buff);
				break;
			}
			case '-':
			{
				char buff[16] = {0};
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
				char _buff[256] = {0};
				char *buff = _buff;
				while(isalpha(c = getNextChar(stream)) ){
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
					if(c == 'x'){
						int tmp = fgetc(stream);
						ungetc(tmp,stream);
						if(!isalpha(tmp)){
							char buff[8];
							buff[0] = c; buff[1] = 0;
							append(head,butt,variable,buff);
							break;
						}
					}
					
					{
						char _buff[256] = {0};
						char *buff = _buff;
						*buff++ = c;
						while(isalpha(c = fgetc(stream)) || c == '_' || isdigit(c) || c == '{' || c == '}'){
							*buff++ = c;
						}
						ungetc(c,stream);
						*buff = 0;
						append(head,butt,variable,_buff);
					}
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
#if DEBUG == 1
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
		}
		now = now->next;
	}
}
#endif
extern Poly _parser(Node *head,Node *tail,BlackBoard blackboard);

Poly _parser(Node *head,Node *tail,BlackBoard blackboard){
	if(head == NULL || head == tail){
		return nullPoly;
	}
	Node *now = head;
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
		if(s + 1 != capacity){
			capacity = s + 1;
			ptr = realloc(ptr,sizeof(Poly) * capacity);
		}
		ptr[s] = _parser(head,now,blackboard);
		if(!isNullPoly(ptr[s])){
			s++;
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
	
	Poly retval = {
		.ptr.items = malloc(sizeof(Item)) 
		//For some reason, when I pass sizeof(Item) to malloc, allocated space get over written.
		//This must be a bug of compiler!!!!!!! Not my fault!
	};
	initK(retval.ptr.items[0].coefficient);
	copyK(retval.ptr.items[0].coefficient , K_1);
	retval.ptr.items[0].degrees = NULL;
	retval.ptr.items[0].size = 0;
	setPolySize(retval,1);
	setPolyType(retval,LEX);
	
	now = head;
	while(now != tail && now){
		switch(now->type){
			case Command:{
				const char *str = now->str;
				if(!strcmp(str,"frac")){
					if(now->next == NULL || now->next->next == NULL ){
						fprintf(stderr,"Invalid \\frac declaration\n");
						DIE;
					}
					Node *nBunsi = now->next;
					Node *nBunbo = now->next->next;
					now = now->next->next->next;
					Poly bunbo = _parser(nBunbo,nBunbo->next,blackboard);
					Poly bunsi = _parser(nBunsi,nBunsi->next,blackboard);
					if(bunbo.size == 1 && bunsi.size == 1 && bunbo.ptr.items[0].size == 0 && bunsi.ptr.items[0].size == 0){
						Item w = {
							.size = 0,
							.degrees = NULL
						};
						initK(w.coefficient);
						divK(w.coefficient,bunsi.ptr.items[0].coefficient,bunbo.ptr.items[0].coefficient);
						Poly mulDis = item2Poly(w);
						Poly tmp = polyMul(retval,mulDis);polyFree(mulDis);	polyFree(retval);
						retval = tmp;
					}else{
						DIE;
					}
				}else{
					now = now->next;
					if(now == NULL || now->type != Block){
						fprintf(stderr,"\'{\' or \'(\' is expected after a call of function\n");
						DIE;
					}
					blackboard = sortBlackBoard(blackboard);
					Poly p = callBuiltInFunc(str,_parser(*((Node **)&now->str),NULL,blackboard),blackboard);
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
					now = now->next;
				}
				break;
			}
			case Block:{
				Poly p = _parser(*((Node **)&now->str),NULL,blackboard);
				Poly tmp = polyMul(retval,p);
				polyFree(p);
				polyFree(retval);
				retval = tmp;
				now = now->next;
				break;
			}
			case Number:{
				K val;
				initK(val);
				str2K(val,now->str);
				int i = 0;
				for(i = 0;i < polySize(retval);i++){
					mulK(retval.ptr.items[i].coefficient,retval.ptr.items[i].coefficient,val);
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
							N n;
							switch(now->type){
								case Block:{
									Poly tmp = _parser(*((Node **)&now->str),NULL,blackboard);
									n = (N)(poly2Double(tmp) + 0.5);
									polyFree(tmp);
								}break;
								case Number:{
									n = (N) atol(now->str);
								}break;
								default : {
									fprintf(stderr,"After \'%s\', a number or block must follow",now->str);
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
					Item item = {
						.size = (index + 1),
						.degrees = calloc(index + 1,sizeof(N))
					};
					copyK(item.coefficient,K_1);
					item.degrees[index] = degrees;
					
					Poly mulDis = {
						.ptr.items = malloc(sizeof(Item))
					};
					mulDis.ptr.items[0] = item;
					setPolySize(mulDis,1);
					setPolyType(mulDis,LEX);
					
					Poly tmp = polyMul(retval,mulDis);
					polyFree(retval);
					polyFree(mulDis);
					retval = tmp;
				}else{
					blackboard = sortBlackBoard(blackboard);
					Poly mulDis = findFromBlackBoard(blackboard,now->str,strlen(now->str));
					if(isNullPoly(mulDis) ) {fprintf(stderr,"Variable %s undefined.\n",now->str);DIE;}
					if(polyType(mulDis) == ARRAY){
						polyFree(retval);
						return mulDis;
					}
					if(polyType(mulDis) != LEX){
						Poly temp = polySort(mulDis,LEX); polyFree(mulDis);
						mulDis = temp;
					}
					Poly tmp = polyMul(retval,mulDis); polyFree(retval); polyFree(mulDis);
					retval = tmp;
					now = now->next;
				}
				break;
			}
			default :{
				fprintf(stderr,"%s\n",now->str);DIE;
			}
		}
	}
	return retval;
}

size_t _node2str(Node *head,Node *tail,char *buff,size_t index,size_t size){
	while(head != tail && head){
		switch(head->type){
			case Command:{
				if(!strcmp(head->str,"_")){
					if(index + 2 >= size) {DIE;}
					buff[index++] = '_';
					buff[index] = 0;
					break;
				}
				break;
			}
			case Block:{
				index = _node2str(*((Node **)&head->str),NULL,buff,index,size);
				break;
			}
			case Variable:{
				strcpy(&buff[index],head->str);
				index += strlen(head->str);
				break;
			}
			case Number:{
				strcpy(&buff[index],head->str);
				index += strlen(head->str);
				break;
			}
		}
		head = head->next;
	}
	return index;
}

Definition takeDefinitionFromNode(Node *node,BlackBoard blackboard){
	Node *now = node;
	if(node->type == Variable && node->next->type == Command && !strcmp(node->next->str,"=")){
		if(!strcmp(node->str,"x")){
			fprintf(stderr,"You cant define variable named \"x\".\n");
			DIE;
		}
		return mkDefinition(node->str,strlen(node->str),_parser(now->next->next,NULL,blackboard));
	}else{
		char buff[2];
		buff[0] = 0;
		return mkDefinition(buff,0,_parser(node,NULL,blackboard));
	}
}

Definition parser(FILE *stream,BlackBoard blackboard){
	int c;
	while((c = fgetc(stream))!=EOF){
		ungetc(c,stream);
		Node *nodes = __parser(stream);
		if(nodes == NULL){
			return nullDefinition;
		}
		#if DEBUG == 1
			fprintf(stderr,"Input : ");
			_print_parsed_tex(nodes,stderr);
			fprintf(stderr,"\n");
			
			Definition def = takeDefinitionFromNode(nodes,blackboard);
			fprintf(stderr,"Parsed as : ");
			polyPrint(def.poly,stderr);
			fprintf(stderr,"\n");
			return def;
		#endif
		return takeDefinitionFromNode(nodes,blackboard);
	}
	return nullDefinition;
}

