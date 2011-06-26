#!/bin/bash -e

PREFIX="../kern/obj/"; mkdir -p $PREFIX
DEPS=(backtrace.o interrupt.o kmalloc.o main.o printk.o switch.o syscall.o task.o ksyms.o info.o)
DEPS=${DEPS[@]/#/${PREFIX}/}

../utils/archive $3 ${DEPS}
