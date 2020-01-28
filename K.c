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

/*
const K JOHO_NO_TANIGEN = {
	.numerator = 1,
	.denominator = 1
};

const K KAHO_NO_TANIGEN = {
	.numerator = 0,
	.denominator = 1
};
const K JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN  = {
	.numerator = -1,
	.denominator = 1
};
*/

K JOHO_NO_TANIGEN;
K KAHO_NO_TANIGEN;
K JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN;

void initConst(){
	mpq_set_si(JOHO_NO_TANIGEN,1,1);
	mpq_set_si(KAHO_NO_TANIGEN,0,1);
	mpq_set_si(JOHO_NO_TANIGEN_NO_KAHO_NO_GYAKUGEN,-1,1);
}
void initK(K k){
	mpq_init(k);
}
void niceQ(Q q){
	mpq_canonicalize(q);
}
void freeQ(Q q){
	mpq_clear(q);
}
int isQNegative(Q m){
    return (mpq_sgn(m) < 0) ? 1 : 0;
}
char * toString(Q q,char *buff){
    char *str = buff;
    sprintf(buff,"%e",mpq_get_d(q));
    return str;
}
void copyK(K to,const K from){
	mpq_init(to);
	mpq_set(to,from);
}
void freeK(K k){
	freeQ(k);
}
void str2K(K val,const char *str){
	mpq_init(val);
	mpq_set_d(val,atof(str));
}
char * K2str(const K k,char *buff){
	char *__buff = mpq_get_str(NULL,10,k);
	char *slash = strchr(__buff,'/');
	if(slash == NULL){
		strcpy(buff,__buff);
		sprintf(buff,"%s",__buff);
	}else{
		*slash = 0;
		sprintf(buff,"\\frac{%s}{%s}",__buff,(slash+1));
	}	
	free(__buff);
	return buff;
}

double K2double(const K k){
	return mpq_get_d(k);
}

void addK(K val,const K v1,const K v2){
	K _v1,_v2;
	copyK(_v1,v1);
	copyK(_v2,v2);
	mpq_add(val,v1,v2);
	freeK(_v1);
	freeK(_v2);
	niceQ(val);
}
void subK(K val,const K v1,const K v2){
K _v1,_v2;
	copyK(_v1,v1);
	copyK(_v2,v2);
	mpq_sub(val,v1,v2);
	freeK(_v1);
	freeK(_v2);
	niceQ(val);
}
void mulK(K val,const K v1,const K v2){
K _v1,_v2;
	copyK(_v1,v1);
	copyK(_v2,v2);
	mpq_mul(val,v1,v2);
	freeK(_v1);
	freeK(_v2);
	niceQ(val);
}
void divK(K val,const K v1,const K v2){
K _v1,_v2;
	copyK(_v1,v1);
	copyK(_v2,v2);
	mpq_div(val,v1,v2);
	freeK(_v1);
	freeK(_v2);
	niceQ(val);
}
int cmpK(const K v1,const K v2){
	return mpq_cmp(v1,v2);
}

