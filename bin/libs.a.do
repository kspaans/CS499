#!/bin/bash -e

PREFIX="../lib/obj/"; mkdir -p $PREFIX
DEPS=(lib.o memcpy.o strlen.o memset.o printf.o string.o msg.o iovec.o)
DEPS=${DEPS[@]/#/${PREFIX}/}

../utils/archive $3 ${DEPS}
