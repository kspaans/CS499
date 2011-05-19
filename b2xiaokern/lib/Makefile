all: libbwio.a

include ../defs.mk
-include bwio.dep
-include io.dep
-include lib.dep
-include cpu.dep
-include libs.dep
-include printf.dep

libbwio.a: io.o lib.o bwio.o cpu.o libs.o printf.o
	$(AR) $(ARFLAGS) $@ $?
	$(AR) -s $@

clean:
	-rm -f *.s *.dep *.a *.o
