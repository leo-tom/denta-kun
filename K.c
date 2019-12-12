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

Q mkQ(int64_t numerator,int64_t denominator)
{
	Q this;
    this.numerator = numerator;
    this.denominator = denominator;
    return this;
}
int64_t _gcd(int64_t m,int64_t n){
    if(n == 0){
        return m;
    }
    return _gcd(n,m%n);
}

Q niceQ(Q this){
    int64_t gcd = _gcd((this.numerator >= this.denominator) ? this.numerator : this.denominator,
                         (this.numerator < this.denominator) ? this.numerator : this.denominator);
    this.numerator /= gcd;
    this.denominator /= gcd;
    if(this.numerator < 0 && this.denominator < 0){
        this.numerator *= -1;
        this.denominator *= -1;
    }else if((this.numerator >= 0 && this.denominator < 0 )){
        this.numerator *= -1;
        this.denominator *= -1;
    }
    return this;
}
int isQNegative(Q this){
    this = niceQ(this);
    return this.numerator < 0;
}
char * toString(Q this,char *buff){
    char *str = buff;
    Q current = this;
    int E = this.numerator / this.denominator;

    if(this.numerator > this.denominator){
        while(current.numerator > current.denominator){
            current.denominator *= 10;
            current = niceQ(current);
            E++;
        }
        E--;
        current.denominator /= 10;
    }else{
        while(current.numerator < current.denominator){
            current.numerator *= 10;
            current = niceQ(current);
            E--;
        }
    }
    if(isQNegative(this)){
    	str = stpcpy(str,"-");
    }
    int64_t integerPart = current.numerator / current.denominator;
    current.numerator = current.numerator % current.denominator;
    current.numerator *= 10;
    current = niceQ(current);
    char c[2] = {'0','\0'};
    if(integerPart <= 9){
        c[0] += integerPart;
    }
    str = stpcpy(str,c);
    str = stpcpy(str,".");
    size_t size = 6; /*How many chars are gonna be displayed*/
    while(size-- > 0){
        integerPart = current.numerator / current.denominator;
        current.numerator = current.numerator % current.denominator;
        current.numerator *= 10;
        current = niceQ(current);
        c[0] = '0';
        if(integerPart <= 9){
            c[0] += integerPart;
        }
        str = stpcpy(str,c);
    }
    str = stpcpy(str,"e");
    char _buff[128];
    sprintf(_buff,"%d",E);
    str = stpcpy(str,_buff);
    return buff;
}

Q operatorAdd(const Q this,const Q m){
    const Q larger = (this.denominator >= m.denominator) ? this : m;
    const Q smaller = (this.denominator < m.denominator) ? this : m;
    const int64_t denomGCD = _gcd(larger.denominator,smaller.denominator);
    int64_t numeratorL = this.numerator * (m.denominator/denomGCD);
    int64_t numeratorR = m.numerator * (this.denominator/denomGCD);
    Q ret = mkQ(numeratorL + numeratorR,this.denominator * (m.denominator/denomGCD));
    return niceQ(ret);
}
Q operatorSub(const Q this,const Q m){
    const Q larger = (this.denominator >= m.denominator) ? this : m;
    const Q smaller = (this.denominator < m.denominator) ? this : m;
    const int64_t denomGCD = _gcd(larger.denominator,smaller.denominator);
    int64_t numeratorL = this.numerator * (m.denominator/denomGCD);
    int64_t numeratorR = m.numerator * (this.denominator/denomGCD);
    Q ret = mkQ(numeratorL - numeratorR,this.denominator * (m.denominator/denomGCD));
    return niceQ(ret);
}

Q operatorMul(const Q this,const Q m){
    Q ret = mkQ(this.numerator * m.numerator,this.denominator * m.denominator);
    return niceQ(ret);
}
Q operatorDiv(const Q this,const Q m){
    Q ret = mkQ(this.numerator * m.denominator,this.denominator * m.numerator);
    return niceQ(ret);
}

K str2K(const char *str){
	char *dot = strchr(str,'.');
	Q retval = mkQ(0,1);
	int isNegative = 0;
	if(str[0] == '-'){
		 isNegative = 1;
		 str++;
	}
	int64_t base;
	if(dot == NULL){
		base = (strlen(str) - 1) * 10;
	}else{
		base = 10 * ( (int64_t) (dot - str - 1));
	}
	if(base == 0){
		base = 1;
	}
	while(isdigit(*str)){
		retval.numerator += (*str++ - '0') * base;
		base /= 10;
	}
	if(dot == NULL){
		goto ret;
	}
	str++;
	base = 10;
	while(isdigit(*str)){
		Q add = mkQ((*str++ - '0'),base);
		retval = operatorAdd(retval,add);
		base *= 10;
	}
	ret:
	if(isNegative){
		retval.numerator *= -1;
	}
	return niceQ(retval);	
}
char * K2str(const K k,char *buff){
	K tmp = niceQ(k);
	sprintf(buff,"\\frac{%ld}{%ld}",tmp.numerator,tmp.denominator);
	return buff;
}

double K2double(const K k){
	return ((double)k.numerator)/((double)k.denominator);
}

K addK(const K v1,const K v2){
	return operatorAdd(v1,v2);
}
K subK(const K v1,const K v2){
	return operatorSub(v1,v2);
}
K mulK(const K v1,const K v2){
	return operatorMul(v1,v2);
}
K divK(const K v1,const K v2){
	return operatorDiv(v1,v2);
}
int cmpK(const K v1,const K v2){
	K diff = subK(v1,v2);
	if(isQNegative(diff)){
		return -1;
	}else if(diff.numerator == 0){
		return 0;
	}else{
		return 1;
	}
}

