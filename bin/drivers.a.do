#!/bin/bash -e

PREFIX="../drivers/obj/"; mkdir -p $PREFIX
DEPS=(eth.o intc.o leds.o timers.o uart.o wd_timer.o gpio.o cpu.o mmu.o)
DEPS=${DEPS[@]/#/${PREFIX}/}

../utils/archive $3 ${DEPS}
