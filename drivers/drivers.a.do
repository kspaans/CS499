DEPS="eth.o intc.o leds.o timers.o uart.o wd_timer.o"
redo-ifchange ${TOP}/utils/archive ${DEPS}

${TOP}/utils/archive $3 ${DEPS}
