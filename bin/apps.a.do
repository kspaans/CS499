#!/bin/bash -e

PREFIX="../apps/obj/"
DEPS=(gameoflife.o init.o)

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/archive ${DEPS}

../utils/archive $3 ${DEPS}
