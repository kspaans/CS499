CC = arm-linux-gnueabi-gcc

# Standard options
CFLAGS  += -Wall -Werror -Iinclude -std=gnu99

# ARMv7 instruction set, Cortex-A8 tuning
CFLAGS  += -march=armv7-a -mtune=cortex-a8

# Keep frame pointers
CFLAGS  += -fno-omit-frame-pointer -mapcs-frame

# Make assembly output more readable
CFLAGS  += -fverbose-asm

# No unqualified builtin functions
CFLAGS  += -fno-builtin

# Do not link in glibc
LDFLAGS += -nostdlib

# Dump link map for inspection
LDFLAGS += -Wl,-Map=$@.map

# Custom linker script
LDFLAGS += -T omap3.ld

# Disable demand-pageable
LDFLAGS += -n

# Link libgcc for compiler-generated function calls
LDLIBS  += -lgcc

# Debugging disabled by default
ifeq ($(DEBUG),1)
  CFLAGS += -g -DDEBUG -Wno-unused-function
endif

all: bin/kernel

bin/kernel: obj/kernel.o obj/bwio.o obj/omap3.o obj/vectors.o

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
