#!/bin/bash

FILE=$(dirname $(dirname $1))/$(basename $1).c
redo-ifchange ${TOP}/utils/compile ${FILE}

${TOP}/utils/compile -MD -MF $3.deps.tmp -o $3 ${FILE}
DEPS=$(sed -e "s?^$3: ??" -e "N;" -e 's/\\//g' < $3.deps.tmp)

rm -f $3.deps.tmp
redo-ifchange ${DEPS}

