mkdir -p obj

DEPS="obj/eth.o obj/intc.o obj/leds.o obj/timers.o obj/uart.o obj/wd_timer.o obj/gpio.o"
redo-ifchange ${TOP}/utils/archive ${DEPS}

${TOP}/utils/archive $3 ${DEPS}
