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

if [ -z $RULE ]
then
	RULE=1
fi

if [ $# -ge 1 ]
then
	RULE="$1"
fi

if [ -z $OUTFILE ]
then
	OUTFILE="out-$RULE.png"
fi

if [ -z $SIZE ]
then
	SIZE='256'
fi
if [ -z $SEED ]
then
	SEED='-1'
fi

if [ -z $DENSITY ]
then
	DENSITY=50
fi

if [ -z $PRINT_POLY ]
then
	PRINT_POLY=0
fi

if [ -z $VARIABLE_SIZE ]
then
	export VARIABLE_SIZE=3
fi

if [ $VARIABLE_SIZE -eq '3' ]
then
	CONVERT_STR='s/x_{0}/__{2}/g; s/x_{1}/__{0}/g; s/x_{2}/__{1}/g; s/__/x_/g'
elif [ $VARIABLE_SIZE -eq '5' ]
then
	CONVERT_STR='s/x_{0}/__{4}/g; s/x_{1}/__{2}/g; s/x_{2}/__{0}/g; s/x_{3}/__{1}/g; s/x_{4}/__{3}/g; s/__/x_/g'
else
	echo error
	exit 1
fi

FUNCTION=$(dentakun-tool function-maker $RULE | sed "$CONVERT_STR")

if [ $PRINT_POLY -ne 0 ]
then
	echo $FUNCTION
	exit 0
fi

FIFO_FILE="_FIFO_PBMAKER_$PPID"

mkfifo $FIFO_FILE

#to make picture huge, set size 13824 and rewrite code.
if which convert > /dev/null
then
	echo "P1
$SIZE $SIZE" | cat /dev/stdin $FIFO_FILE | convert pbm:- png:$OUTFILE &
else
	echo "P1
$SIZE $SIZE" | cat /dev/stdin $FIFO_FILE > $OUTFILE &
fi

echo $($AWK -v "density=$DENSITY" -v "size=$SIZE" -v "seed=$SEED" 'BEGIN{
	if(size <= 0){
		size = 256;
	}
	if(seed < 0){
		printf("BCA_INITIAL_STATE = (");
		for(i = 0;i < size-1;i++){
			if(i == size / 2){
				printf("1,");
			}else{
				printf("0,");
			}
		}
		printf("0) \\\\\n");
	}else if(seed >= 0){
		printf("BCA_INITIAL_STATE = (");
		srand(seed);
		for(i = 0;i < size-1;i++){
			if((rand()*1000)%100 >= density){
				printf("0,");
			}else{
				printf("1,");
			}
		}
		if((rand()*1000)%100 >= density){
			printf("0) \\\\\n");
		}else{
			printf("1) \\\\\n");
		}
	}
}
') 'f=' "$FUNCTION" '\\ \PP(\BCA(f)) \\' | bentakun | sed 'y/(),/   /; s/ //g ; s/\\//g ; /^$/d ' > $FIFO_FILE
rm -f $FIFO_FILE


