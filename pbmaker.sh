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
RULE=1
if [ $# -ge 1 ]
then
	RULE="$1"
fi

if [ $RULE -lt 0 -o $RULE -ge 256 ]
then
	echo 'Rule has to be a integer between 0 to 255.'
	exit 1
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

mkfifo $FIFO_FILE

#to make picture huge, set size 13824 and rewrite code.
if which magick > /dev/null
then
	echo "P1
$SIZE $SIZE" | cat /dev/stdin $FIFO_FILE | magick convert pbm:- $OUTFILE &
else
	echo "P1
$SIZE $SIZE" | cat /dev/stdin $FIFO_FILE > $OUTFILE &
fi

awk -v "rule=$RULE" -v "size=$SIZE" -v "seed=$SEED" 'BEGIN{
	if(size <= 0){
		size = 256;
	}else if(seed < 0){
		printf("BCA_INITIAL_STATE = ");
		for(i = 0;i < size-1;i++){
			if(i == size / 2){
				printf("1,");
			}else{
				printf("0,");
			}
		}
		printf("0 \\\\\n");
	}
	if(seed >= 0){
		printf("BCA_INITIAL_STATE = ");
		srand(seed);
		for(i = 0;i < size-1;i++){
			printf("%d,",(rand()*10)%2);
		}
		printf("%d \\\\\n",(rand()*10)%2);
	}
	subscript = 0;
	for(i = 0;i < 8;i++){
		if(and(rule,lshift(1,i))){
			printf("f%d = ",subscript++);
			if(and(i , 2)){
				printf("( x_0 + 0 )");
			}else{
				printf("( x_0 + 1 )");
			}
			if(and(i , 4)){
				printf("( x_1 + 0 )");
			}else{
				printf("( x_1 + 1 )");
			}
			if(and(i , 1)){
				printf("( x_2 + 0 )");
			}else{
				printf("( x_2 + 1 )");
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
	printf("\\PP(\\BCA(f)) \\\\\n");
	#printf("\\PP(f) \\\\\n");
	#printf("\\BCA(f) \\\\\n");
}
' | bentakun | sed 'y/(),/   /; s/ //g ; /^$/d' >> $FIFO_FILE

rm -f $FIFO_FILE


