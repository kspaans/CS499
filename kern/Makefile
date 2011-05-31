S = ..

TARGET = $(S)/bin/kern.elf

all: $(TARGET)

OBJS = obj/backtrace.o obj/interrupt.o obj/kmalloc.o obj/main.o obj/omap3.o obj/printk.o obj/switch.o obj/syscall.o obj/task.o obj/userprog.o
LIBS = $(S)/bin/libs.a $(S)/bin/drivers.a $(S)/bin/servers.a

include ../defs.mk
-include $(OBJS:.o=.dep)
