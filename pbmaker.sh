#! /bin/sh -
make DEBUG=0 BOOLEAN=1
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

if which magick > /dev/null
then
	echo "P1
$SIZE $SIZE" | cat /dev/stdin $FIFO_FILE | magick convert pbm:- "out-$RULE.png" &
else
	echo "P1
$SIZE $SIZE" | cat /dev/stdin $FIFO_FILE > "out-$RULE.pbm" &
fi



awk -v "rule=$RULE" -v "size=$SIZE" -v "seed=$SEED" 'BEGIN{
	if(size <= 0){
		size = 256;
	}
	if(seed >= 0){
		printf("BCA_INITIAL_STATE = ");
		srand(seed);
		for(i = 0;i < size-1;i++){
			printf("%d , ",(rand()*10)%2);
		}
		printf("%d \\\\\n",(rand()*10)%2);
	}
	subscript = 0;
	if(and(rule, 1)){
		printf("f%d = 1 \\\\\n",subscript++);
	}
	for(i = 1;i < 8;i++){
		if(and(rule,lshift(1,i))){
			printf("f%d = (",subscript++);
			if(and(i , 2)){
				printf("x_0 ");
			}
			if(and(i , 4)){
				printf("x_1 ");
			}
			if(and(i , 1)){
				printf("x_2 ");
			}
			printf("+ 1 ) + 1 \\\\\n");
		}
	}
	printf("f = ");
	for(i = subscript - 1;i > 0;i--){
		printf("f%d + ",i);
	}
	printf("f0 \\\\\n");
	printf("\\PP(\\BCA(f)) \\\\\n");
}
' | ./dentakun | sed 'y/(),/   /; s/ //g ; /^$/d' >> $FIFO_FILE

rm -f $FIFO_FILE


