#!/bin/bash -e

PREFIX="../apps/obj/"; mkdir -p $PREFIX
DEPS=(gameoflife.o init.o)
DEPS=${DEPS[@]/#/${PREFIX}/}

../utils/archive $3 ${DEPS}
