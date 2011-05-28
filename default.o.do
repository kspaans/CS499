#!/bin/bash

if [ -e $1.S ] ; then
    FILE=$1.S
else
    FILE=$1.s
fi

redo-ifchange ${TOP}/utils/assemble ${FILE}
${TOP}/utils/assemble -o $3 ${FILE}
