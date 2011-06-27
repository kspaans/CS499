#!/bin/bash -e

DEPS=(lib/lib.c lib/memcpy.S lib/strlen.S
      lib/memset.S lib/printf.c lib/string.c
      lib/msg.c lib/iovec.c)

../utils/archive $3 ${DEPS[@]/#/../}
