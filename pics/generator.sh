#! /bin/sh -
awk 'BEGIN{for(i = 0;i < 256;i++){print i}}' | while read RULE
do
	DENTAKUN_CMD='BCA_PERIODIC=1 \\' OUTPUT="$RULE.png" SIZE=128 dentakun-tool pbmaker $RULE
done
convert -delay 100 *.png bca.gif
