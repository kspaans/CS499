S = ..

TARGET = $(S)/bin/libbwio.a

all: $(TARGET)

OBJS = obj/cpu.o obj/lib.o obj/memcpy.o obj/printf.o obj/ip.o obj/string.o obj/bwio.o

include ../defs.mk
-include $(OBJS:.o=.dep)
