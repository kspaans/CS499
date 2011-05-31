S = ..

TARGET = $(S)/bin/servers.a

all: $(TARGET)

OBJS = obj/clock.o obj/console.o obj/net.o

include ../defs.mk
-include $(OBJS:.o=.dep)
