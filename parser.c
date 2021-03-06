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
	Block
};

typedef struct _Node{
	enum NodeType type;
	char str[24];
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

//return next node
Node * freeNode(Node *node);
void freeNodes(Node *node);

Poly _parser(Node *head,Node *tail,BlackBoard *blackboard);

Node * __parser(FILE *stream){
	int c;
	
	Node *head,*butt;
	head = butt = NULL;
	char _buff[24];
	size_t counter = sizeof(_buff) - 1;
	
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
			case '^':
			{
				DIE;
			}		
			case '=':
			case '+':
			case '_':
			case ',':
			{
				char buff[2];
				buff[0] = c; buff[1] = 0;
				append(head,butt,Command,buff);
				break;
			}
			case '-':
			{
				char buff[3];
				buff[0] = '+'; buff[1] = 0;
				append(head,butt,Command,buff);
				strcpy(buff,"-1");
				append(head,butt,Number,buff);
				break;
			}
			case '\\':
			{
				if((c = getNextChar(stream)) == '\\'){
					goto ret;
				}
				ungetc(c,stream);
				char *buff = _buff;
				counter = sizeof(_buff) - 1;
				while(isalpha(c = fgetc(stream)) || isdigit(c)){
					*buff++ = c;
					counter--;
					if(counter == 0){
						fprintf(stderr,"Too long function name.\n");
						DIE;
					}
				}
				*buff = 0;
				ungetc(c,stream);
				append(head,butt,Command,_buff);
				break;
			}
			case '{':
			case '(':
			{
				Node *new = malloc(sizeof(Node));
				new->type = Block;
				new->next = NULL;
				void *p = (void *)__parser(stream);
				memcpy(new->str,&p,sizeof(void *));
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
					goto ret;
				}
			default :
			{
				if(isdigit(c) || c == '-'){
					char *buff = _buff;
					counter = sizeof(_buff) - 2;
					*buff++ = c;
					while(isdigit(c = fgetc(stream)) || c == '.' || c == 'E' || c == 'e' ){
						*buff++ = c;
						counter--;
						if(counter == 0){
							fprintf(stderr,"Too long function name.\n");
							DIE;
						}
					}
					ungetc(c,stream);
					*buff = 0;
					append(head,butt,Number,_buff);
				}else if(isalpha(c)){
					char *buff = _buff;
					counter = sizeof(_buff) - 2;
					*buff++ = c;
					while(isalpha(c = fgetc(stream)) || isdigit(c)){
						*buff++ = c;
						counter--;
						if(counter == 0){
							fprintf(stderr,"Too long function name.\n");
							DIE;
						}
					}
					ungetc(c,stream);
					*buff = 0;
					if(!strcmp(_buff,"x")){
						c = getNextChar(stream);
						if(c != '_'){
							ungetc(c,stream);
						}else{
							//DO SPECIAL BEHAVIOR
							//name this variable "$number"
							//where number is subscript of x
							if((c = getNextChar(stream)) == '{'){
								buff = _buff;
								counter = sizeof(_buff) - 3;
								*buff++ = '$';
								c = getNextChar(stream);
								if( (!isdigit(c)) && c != '-'){
									DIE;
								}
								*buff++ = c;
								while(isdigit(c = getNextChar(stream))){
									*buff++ = c;
									counter--;
									if(counter == 0){
										DIE;
									}
								}
								if(c != '}'){
									DIE;
								}
								*buff = 0;
							}else if(isdigit(c)){
								_buff[0] = '$';
								_buff[1] = c;
								_buff[2] = 0;
							}else{
								fprintf(stderr,"\"x_\" must not be followed by %c.",c);
								DIE;
							}
						}
					}
					append(head,butt,Variable,_buff);
				}else{
					fprintf(stderr,"Unknown char\'%c\'[%x]\n",c,c);
					DIE;
				}
				break;
			}	
		}
	}
	ret:
	return head;
}
Node * unwrapBlock(Node *node){
	void *retval;
	memcpy(&retval,node->str,sizeof(void *));
	return retval;
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
				Node *ptr = unwrapBlock(now);
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
	}else if(!strcmp(name,"FBL")){
		if(head->next == NULL ){
			fprintf(stderr,"FBL function takes an argument.\n");
			DIE;
		}
		Node *now = unwrapBlock(head->next);
		size_t capacity = 8;
		size_t i = 0;
		Term *ptr = malloc(sizeof(Term)*capacity);
		ptr[i].sizu = 0;
		ptr[i].deg.val = 0;
		copyK(ptr[i].coefficient,K_1);
		while(now){
			if(now->str[0] == '$'){
				int64_t index = SUBSHIFT + atoi(&(now->str[1]));
				if(index >= sizeof(N)*8){
					if(termSize(ptr[i]) <= sizeof(N)*8){
						N tmp = ptr[i].deg.val;
						N *p = calloc(sizeof(N),(index + 1)/(sizeof(N)*8) + 1);
						p[0] = tmp;
						ptr[i].deg.ptr = p;
					}else if((termSize(ptr[i])/(sizeof(N)*8) ) <= ((index+1)/(sizeof(N)*8)) ){
						N *p = calloc(sizeof(N),(index + 1)/(sizeof(N)*8) + 1);
						memcpy(p,ptr[i].deg.ptr,sizeof(N)*(termSize(ptr[i])/(sizeof(N)*8) + 1) );
						free(ptr[i].deg.ptr);
						ptr[i].deg.ptr = p;
					}
				}
				if(index >= termSize(ptr[i])){
					setTermSize(ptr[i],(index+1));
				}
				setTermDegree(ptr[i],index,1);
			}else if(!strcmp(now->str,"+")){
				i++;
				if(i >= capacity){
					capacity *= 2;
					ptr = realloc(ptr,sizeof(Term)*capacity);
				}
				ptr[i].sizu = 0;
				ptr[i].deg.val = 0;
				copyK(ptr[i].coefficient,K_1);
			}else if(now->type == Number){
				K muldis;
				str2K(muldis,now->str);
				mulK(ptr[i].coefficient,ptr[i].coefficient,muldis);
			}
			now = freeNode(now);
		}
		i++;
		Poly retval = {
			.size = i
		};
		retval.ptr.terms = realloc(ptr,sizeof(Term)*i);
		memset(head->next->str,0,sizeof(void *));
		*next = head->next->next;
		return _polySort(retval);
	}
	DIE;
}

int isSpecialFunction(const char * const name){
	if(!strcmp(name,"IF")
	|| !strcmp(name,"FOREACH")
	|| !strcmp(name,"WHILE")
	|| !strcmp(name,"frac")
	|| !strcmp(name,"FRAC")
	|| !strcmp(name,"FBL")
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
			fprintf(stderr,"NO!");
			s--;
			DIE;
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
						if(polyType(pLeft) == ARRAY || polyType(pRight) == ARRAY){
							if(polyType(pLeft) == ARRAY && polyType(pRight) == ARRAY){
								size_t size = polySize(pLeft) + polySize(pRight);
								Poly *ptr = malloc(sizeof(Poly) * size);
								memcpy(ptr,unwrapPolyArray(pLeft),sizeof(Poly) * polySize(pLeft));
								memcpy(&ptr[polySize(pLeft)],unwrapPolyArray(pRight),sizeof(Poly) * polySize(pRight));
								free(unwrapPolyArray(pLeft));
								free(unwrapPolyArray(pRight));
								return mkPolyArray(ptr,size);
							}else{
								if(polyType(pLeft) == ARRAY){
									Poly *ptr = unwrapPolyArray(pLeft);
									ptr = realloc(ptr,sizeof(Poly) * (polySize(pLeft) + 1));
									ptr[polySize(pLeft)] = pRight;
									return mkPolyArray(ptr,(polySize(pLeft) + 1));
								}else{
									Poly *ptr = malloc(sizeof(Poly) * (polySize(pRight) + 1));
									ptr[0] = pLeft;
									memcpy(&ptr[1],unwrapPolyArray(pRight),sizeof(Poly) * polySize(pRight));
									return mkPolyArray(ptr,polySize(pRight) + 1);
								}
							}
						}else{
							return _polyAdd(pLeft,pRight);
						}
					}else{
						return pRight;
					}
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
					return _polyMul(_parser(head,now,blackboard),_parser(now->next,tail,blackboard));
				}
				break;
			}
			default:{
				break;
			}
		}
		now = now->next;
	}
	
	Poly retval = polyDup(onePoly);
	
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
						p = callBuiltInFunc(str,_parser(unwrapBlock(now),NULL,blackboard),*blackboard);
						now = now->next;
					}
				}
				if(polyType(p) == ARRAY){
					polyFree(retval);
					return p;
				}else if(isNullPoly(p)){
					polyFree(retval);
					return p;
				}
				retval = _polyMul(retval,p);
				break;
			}
			case Block:{
				Poly p = _parser(unwrapBlock(now),NULL,blackboard);
				if(now->next == NULL && head == now){
					polyFree(retval);
					return p;
				}else if(polyType(p) == ARRAY){
					polyFree(retval);
					return p;
				}
				retval = _polyMul(retval,p);
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
				if(now->str[0] == '$'){
					int64_t index = atoi(&(now->str[1])) + SUBSHIFT;
					Term term;
					setTermSize(term,(index + 1));
					termDegreeAllocator(term);
					setTermDegree(term,index,1);
					copyK(term.coefficient,K_1);
					Poly mulDis = term2Poly(term);
					retval = _polyMul(retval,mulDis);
					now = now->next;
				}else if(!strcmp(now->str,"x")){
					fprintf(stderr,"WTF\n");
					DIE;
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
					retval = _polyMul(retval,mulDis); 
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

Node * freeNode(Node *node){
	if(node == NULL){
		return NULL;
	}
	Node *next = node->next;
	if(node->type == Block){
		freeNodes(unwrapBlock(node));
	}
	free(node);
	return next;
}

void freeNodes(Node *node){
	while(node){
		node = freeNode(node);
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
		#endif
		Poly retval = _parser(nodes,NULL,blackboard);
		freeNodes(nodes);
		#if DEBUG >= 2
			fprintf(stderr,"Parsed as : ");
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


