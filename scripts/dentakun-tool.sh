#!/bin/bash -
DIR="/usr/local/share/dentakun/scripts/"
PRGNAME="$1"
PRGNAME=$(echo "$PRGNAME" | sed 's/\.sh//g').sh
shift
$DIR$PRGNAME $@

