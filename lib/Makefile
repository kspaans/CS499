S = ..

TARGET = $(S)/bin/libs.a

all: $(TARGET)

OBJS = obj/cpu.o obj/lib.o obj/memcpy.o obj/printf.o obj/string.o

include ../defs.mk
-include $(OBJS:.o=.dep)
