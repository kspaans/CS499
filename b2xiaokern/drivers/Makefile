S = ..

TARGET = $(S)/bin/drivers.a

all: $(TARGET)

OBJS = obj/eth.o obj/intc.o obj/leds.o obj/timers.o obj/uart.o obj/wd_timer.o

include ../defs.mk
-include $(OBJS:.o=.dep)
