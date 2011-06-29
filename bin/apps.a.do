#!/bin/bash -e

DEPS=(apps/gameoflife.c
      apps/shell.c
      apps/init.c)

../utils/archive $3 ${DEPS[@]/#/../}
