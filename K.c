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
char * K2str(K k,char *buff){
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
char * K2strScientific(K k,char *buff){
    char *str = buff;
    sprintf(buff,"%e",mpq_get_d(k));
    return str;
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
}
void subK(K val,const K v1,const K v2){
K _v1,_v2;
	copyK(_v1,v1);
	copyK(_v2,v2);
	mpq_sub(val,v1,v2);
	freeK(_v1);
	freeK(_v2);
}
void mulK(K val,const K v1,const K v2){
K _v1,_v2;
	copyK(_v1,v1);
	copyK(_v2,v2);
	mpq_mul(val,v1,v2);
	freeK(_v1);
	freeK(_v2);
}
void divK(K val,const K v1,const K v2){
K _v1,_v2;
	copyK(_v1,v1);
	copyK(_v2,v2);
	mpq_div(val,v1,v2);
	freeK(_v1);
	freeK(_v2);
}
int cmpK(const K v1,const K v2){
	return mpq_cmp(v1,v2);
}

