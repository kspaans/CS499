#!/bin/bash

Sfile=$(dirname $(dirname $1))/$(basename $1).S
if [ -e $Sfile ] ; then
    FILE=$Sfile
else
    FILE=$1.s
fi

redo-ifchange ${TOP}/utils/assemble ${FILE}
${TOP}/utils/assemble -o $3 ${FILE}
