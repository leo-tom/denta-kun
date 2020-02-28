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

echo $(awk -v "size=$SIZE" -v "seed=$SEED" 'BEGIN{
	if(size <= 0){
		size = 256;
	}
	if(seed < 0){
		printf("BCA_INITIAL_STATE = ");
		for(i = 0;i < size-1;i++){
			if(i == size / 2){
				printf("1,");
			}else{
				printf("0,");
			}
		}
		printf("0 \\\\\n");
	}else if(seed >= 0){
		printf("BCA_INITIAL_STATE = ");
		srand(seed);
		for(i = 0;i < size-1;i++){
			printf("%d,",(rand()*10)%2);
		}
		printf("%d \\\\\n",(rand()*10)%2);
	}
}
') 'f=' $(dentakun-tool function-maker $RULE) '\PP(\BCA(f)) \\' | bentakun | sed 'y/(),/   /; s/ //g ; /^$/d' >> $FIFO_FILE
rm -f $FIFO_FILE


