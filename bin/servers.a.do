#!/bin/bash -e

DEPS=(servers/clock.c servers/console.c servers/net.c
      servers/fs.c servers/genesis.c servers/lock.c)

../utils/archive $3 ${DEPS[@]/#/../}
