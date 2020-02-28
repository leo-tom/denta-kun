#! /bin/sh -
#Copyright 2020, Leo Tomura
#
#This file is part of Dentakun.
#
#Dentakun is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.
#
#Dentakun is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with Dentakun.  If not, see <http://www.gnu.org/licenses/>.
#
FIFO_FILE='_FIFO_PBMAKER_'
if [ $# -eq 1 ]
then
	NUMBER="$1"
else
	echo "function-maker requires 1 variables."
	exit 1
fi

if [ -z $VARIABLE_SIZE ]
then
	VARIABLE_SIZE=$(awk -v "rule=$NUMBER" 'BEGIN{if(rule==1||rule==0){print 1}else{print int(log(log(rule)/log(2))/log(2)+1)}}')
fi

LIMIT=$(awk -v "size=$VARIABLE_SIZE" 'BEGIN{printf("%u", (2^(2^size)) -1 );}')

if [ $VARIABLE_SIZE -lt 0 -o $VARIABLE_SIZE -gt 64 ]
then
	echo 'Input variables has to be x_0 to x_63.'
	exit 1
elif [ $NUMBER -gt $LIMIT ]
then
	echo "$NUMBER >= 2^2^$VARIABLE_SIZE = $LIMIT. It has to be $NUMBER < 2^2^$VARIABLE_SIZE = $LIMIT"
	exit 1
fi
if [ -z $OUTFILE ]
then
	OUTFILE="/dev/stdout"
fi
if [ -z $SUBSHIFT ]
then
	SUBSHIFT=0
fi

awk -v "VARIABLE_SIZE=$VARIABLE_SIZE" -v "NUMBER=$NUMBER" 'BEGIN{
	subscript = 0;
	size = 2^VARIABLE_SIZE;
	for(i = 0;i < size;i++){
		if(and(NUMBER,lshift(1,i))){
			printf("f%d = ",subscript++);
			for(j = 0;j < VARIABLE_SIZE;j++){
				if(and(i , lshift(1,j))){
					printf("(x_%d + 0)",j);
				}else{
					printf("(x_%d + 1)",j);
				}
			}
			printf("\\\\\n");
		}
	}
	if(subscript == 0){
		printf("f = 0 \\\\\n");
	}else if(subscript == 1){
		printf("f = f0 \\\\\n");
	}else{
		for(i = subscript - 2;i >= 0;i--){
			printf("f%d = f%d + f%d + f%d f%d  \\\\\n",i,i,i+1,i,i+1);
		}
		printf("f = f0 \\\\\n" );
	}
	printf("\\PP(f) \\\\\n");
}
' 
#| bentakun -s $SUBSHIFT > $OUTFILE

