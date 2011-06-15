PREFIX="../drivers/obj/"
DEPS=(eth.o intc.o leds.o timers.o uart.o wd_timer.o gpio.o)

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/archive ${DEPS}

../utils/archive $3 ${DEPS}
