#!/bin/bash -e

FILE=$(dirname $(dirname $1))/$(basename $1).c
redo-ifchange utils/compile ${FILE}

utils/compile -MD -MF $3.deps.tmp -o $3 ${FILE}
DEPS=$(sed -e "s?^$3: ??" -e "N;" -e 's/\\//g' < $3.deps.tmp)

rm -f $3.deps.tmp
redo-ifchange ${DEPS}

