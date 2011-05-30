S = ..

TARGET = $(S)/bin/kern.elf

all: $(TARGET)

OBJS = obj/backtrace.o obj/interrupt.o obj/kmalloc.o obj/main.o obj/omap3.o obj/printk.o obj/switch.o obj/syscall.o obj/task.o obj/userprog.o obj/clock.o obj/console.o
LIBS = $(S)/bin/libbwio.a $(S)/bin/drivers.a

include ../defs.mk
-include $(OBJS:.o=.dep)
