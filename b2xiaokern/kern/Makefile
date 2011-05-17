#
# Kernel
#

PROG = kern

all: $(PROG).elf

include ../defs.mk
-include switch.dep
-include syscall.dep
-include main.dep
-include task.dep
-include interrupt.dep
-include eth.dep

$(PROG).elf: switch.o syscall.o main.o task.o omap3.o interrupt.o eth.o ../lib/libbwio.a
	$(XCC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LDLIBS)

clean:
	-rm -f $(PROG).elf *.dep *.o *.map *.s
