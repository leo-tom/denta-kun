#!/bin/bash -
DIR="/usr/local/share/dentakun/scripts/"
PRGNAME="$1"
PRGNAME=$(echo "$PRGNAME" | sed 's/\.sh//g').sh
if [ -z $VARIABLE_SIZE ]
then
	export VARIABLE_SIZE=3
fi
shift
AWK="$(which gawk) -M "
if [ -z "$AWK" ]
then
	AWK='awk '
fi

export AWK

$DIR$PRGNAME $@

