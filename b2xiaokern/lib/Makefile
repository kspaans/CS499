S = ..

TARGET = $(S)/bin/libbwio.a

all: $(TARGET)

OBJS = obj/io.o obj/lib.o obj/bwio.o obj/cpu.o obj/libs.o obj/printf.o

include ../defs.mk
-include $(OBJS:.o=.dep)
