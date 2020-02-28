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

#if BOOLEAN

void recursivePAC(Poly poly){
	
}

Poly PAC(Poly arg,BlackBoard blackboard){
	if(polyType(arg) != ARRAY || polySize(arg) != 8){
		polyPrint(arg,K2str,stderr);
		fprintf(stderr," is invalid as argument of PAC.\n");
	}
	int i,j,k;
	Poly *array = unwrapPolyArray(arg);
	fprintf(OUTFILE,"char functionName(char v){\n");
	fprintf(OUTFILE,"const char s0 = v;\n");
	fprintf(OUTFILE,"const char s1 = (v << 1) | (v >> 7);\n");
	fprintf(OUTFILE,"const char s2 = (v << 2) | (v >> 6);\n");
	fprintf(OUTFILE,"const char s3 = (v << 3) | (v >> 5);\n");
	fprintf(OUTFILE,"const char s4 = (v << 4) | (v >> 4);\n");
	fprintf(OUTFILE,"const char s5 = (v << 5) | (v >> 3);\n");
	fprintf(OUTFILE,"const char s6 = (v << 6) | (v >> 2);\n");
	fprintf(OUTFILE,"const char s7 = (v << 7) | (v >> 1);\n");
	fprintf(OUTFILE,"return ");
	for(i = 0;i < 8;i++){
		Poly poly = array[i];
		fprintf(OUTFILE,"( (");
		for(j = 0;j < polySize(poly);j++){
			Item item = poly.ptr.items[j];
			if(!cmpK(item.coefficient,K_0)){
				break;
			}	
			fprintf(OUTFILE,"(");
			int isFirstAND = 1;	
			for(k = 0;k < item.size;k++){
				if(item.degrees[k]){
					int shift = (i - k);
					shift = shift < 0 ? 8 + shift : shift;
					if(isFirstAND){
						fprintf(OUTFILE,"s%d",shift);
						isFirstAND=0;
					}else{
						fprintf(OUTFILE,"&s%d",shift);
					}
				}
			}
			if(j + 1 != polySize(poly)){
				fprintf(OUTFILE,")^");
			}else{
				fprintf(OUTFILE,")");
			}	
		}
		if(i + 1 != 8){
			fprintf(OUTFILE,") & (1 << %d)) \n\t|",i);
		}else{
			fprintf(OUTFILE,") & (1 << %d))",i);
		}	
	}
	fprintf(OUTFILE,";\n}\n");
	polyFree(arg);
	return nullPoly;
}
#endif
