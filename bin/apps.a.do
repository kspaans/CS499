#!/bin/bash -e

DEPS=(apps/gameoflife.c
      apps/init.c)

../utils/archive $3 ${DEPS[@]/#/../}
