#!/bin/bash -e

DEPS=(apps/gameoflife.c
      apps/shell.c
      apps/init.c
      apps/test.c
      apps/ipc_bench.c
      apps/memcpy_bench.c)

../utils/archive $3 ${DEPS[@]/#/../}
