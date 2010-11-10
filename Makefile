CC = gcc -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -I/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2/include

# Standard options
CFLAGS  += -Wall -Werror -Iinclude -std=gnu99

# ARMv5TE instruction set, ARM926ej-s tuning
CFLAGS  += -march=armv5te -mtune=arm926ej-s

# Keep frame pointers
CFLAGS  += -fno-omit-frame-pointer

# Make assembly output more readable
CFLAGS  += -fverbose-asm

# No unqualified builtin functions
CFLAGS  += -fno-builtin

# Do not link in glibc
LDFLAGS += -nostdlib

# Dump link map for inspection
LDFLAGS += -Wl,-Map=$@.map

# Custom linker script
LDFLAGS += -T ts7800.ld

# Disable demand-pageable
LDFLAGS += -n

# Link libgcc for compiler-generated function calls
LDLIBS  += -lgcc

# Debugging disabled by default
ifeq ($(DEBUG),1)
  CFLAGS += -g -DDEBUG -Wno-unused-function
endif

all: bin/iotest

bin/iotest: obj/bwio.o obj/iotest.o obj/ts7800.o

bin/% :
	@mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LDLIBS)

obj/%.s : %.c
	@mkdir -p $(shell dirname $@)
	$(CC) -S $(CFLAGS) -o $@ $^

obj/%.o : obj/%.s
	@mkdir -p $(shell dirname $@)
	$(CC) -c $(CFLAGS) -o $@ $^

obj/%.o : %.S
	@mkdir -p $(shell dirname $@)
	$(CC) -c $(CFLAGS) -o $@ $^

obj/%.s : obj/%.c
	@mkdir -p $(shell dirname $@)
	$(CC) -c $(CFLAGS) -o $@ $^

clean:
	rm -rf obj lib bin

.PRECIOUS: obj/%.s obj/%.c
