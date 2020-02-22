#! /bin/sh -
awk 'BEGIN{for(i = 0;i < 256;i++){print i}}' | while read RULE
do
	DENTAKUN_CMD='BCA_REFLECTIVE=1 \\' OUTPUT="$RULE.png" SEED=1 SIZE=512 dentakun-bca-tool $RULE
done
magick convert -delay 100 *.png bca.gif
