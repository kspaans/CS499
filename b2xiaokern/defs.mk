S       = ..
XPREFIX = arm-linux-gnueabi-
XCC     = $(XPREFIX)gcc
AS      = $(XPREFIX)as
LD      = $(XPREFIX)ld
AR      = $(XPREFIX)ar

# Standard options
CFLAGS  += -Wall -I$(S)/include -std=gnu99 -O2

# ARMv7 instruction set, Cortex-A8 tuning
CFLAGS  += -march=armv7-a -mtune=cortex-a8

# Keep frame pointers
CFLAGS  += -fno-omit-frame-pointer -mapcs-frame -marm

# Make assembly output more readable
CFLAGS  += -fverbose-asm

# No unqualified builtin functions
CFLAGS  += -fno-builtin

# Supervisor-mode tasks
CFLAGS  += -DSUPERVISOR_TASKS


# Do not link in glibc
LDFLAGS += -nostdlib

# Dump link map for inspection
LDFLAGS += -Wl,-Map=$@.map

# Custom linker script
LDFLAGS += -T $(S)/omap3.ld

# Disable demand-pageable
LDFLAGS += -n

# Link libgcc for compiler-generated function calls
LDLIBS  += -lgcc

# Debugging disabled by default
ifeq ($(DEBUG),1)
  CFLAGS += -g -DDEBUG -Wno-unused-function
endif


ARFLAGS = rs

# New pattern rules
%.s: %.c
	$(XCC) -S $(CFLAGS) -o $@ $<

%.o: %.s
	$(XCC) -c $(CFLAGS) -o $@ $<

%.o: %.S
	$(XCC) -c $(CFLAGS) -o $@ $<

%.dep: %.c
	$(SHELL) -ec '$(XCC) -S -MT $(<:%.c=%.s) -M $(CFLAGS) $< | sed "s/$*.s/& $@/g" > $@'

%.dep: %.S
	$(SHELL) -ec '$(XCC) -S -M $(CFLAGS) $< | sed "s/$*.S/& $@/g" > $@'

# Cancel .c shortcut rule
%.o: %.c

# Keep .s files around
.PRECIOUS: %.s

upload: $(PROG).elf
	scp $(PROG).elf csclub:/srv/tftp/ARM/b2xiao/$(PROG)
