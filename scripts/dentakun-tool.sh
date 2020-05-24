#!/bin/bash -
DIR="/usr/local/share/dentakun/scripts/"
PRGNAME="$1"
PRGNAME=$(echo "$PRGNAME" | sed 's/\.sh//g').sh
if [ -z $VARIABLE_SIZE ]
then
	export VARIABLE_SIZE=3
fi
shift
if which gawk > /dev/null
then
	export AWK='gawk -M '
else
	export AWK='awk '
fi
$DIR$PRGNAME $@

