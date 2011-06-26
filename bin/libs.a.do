#!/bin/bash -e

PREFIX="../lib/obj/"
DEPS=(lib.o memcpy.o strlen.o memset.o printf.o string.o msg.o iovec.o)

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/archive ${DEPS}

../utils/archive $3 ${DEPS}
