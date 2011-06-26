#!/bin/bash -e

PREFIX="../servers/obj/"; mkdir -p $PREFIX
DEPS=(clock.o console.o net.o fs.o genesis.o)
DEPS=${DEPS[@]/#/${PREFIX}/}

../utils/archive $3 ${DEPS}

