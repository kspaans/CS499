#!/bin/bash -e

DEPS=(drivers/eth.c drivers/intc.c drivers/leds.c
      drivers/timers.c drivers/uart.c drivers/wd_timer.c
      drivers/gpio.c drivers/pmu.c)

../utils/archive $3 ${DEPS[@]/#/../}
