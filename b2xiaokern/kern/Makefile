S = ..

TARGET = $(S)/bin/kern.elf

all: $(TARGET)

OBJS = obj/switch.o obj/syscall.o obj/main.o obj/task.o obj/omap3.o obj/interrupt.o obj/eth.o
LIBS = $(S)/bin/libbwio.a

include ../defs.mk
-include $(OBJS:.o=.dep)
