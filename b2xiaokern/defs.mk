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


# New pattern rules
$(S)/bin/%.elf : $(OBJS) $(LIBS)
	-@mkdir -p $(S)/bin
	$(XCC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(LDLIBS)

$(S)/bin/%.a : $(OBJS)
	-@mkdir -p $(S)/bin
	$(AR) crs $@ $(OBJS)

obj/%.s: %.c obj/%.dep
	-@mkdir -p obj
	$(XCC) -S $(CFLAGS) -o $@ $(filter %.c,$^)

obj/%.o: obj/%.s
	-@mkdir -p obj
	$(XCC) -c $(CFLAGS) -o $@ $<

obj/%.o: %.S obj/%.dep
	-@mkdir -p obj
	$(XCC) -c $(CFLAGS) -o $@ $(filter %.S,$^)

obj/%.dep: %.c
	-@mkdir -p obj
	$(SHELL) -ec '$(XCC) -S -MT $(<:%.c=%.s) -M $(CFLAGS) $< | sed "s/$*.s/& $(subst $@,/,\\/)/g" > $@'

obj/%.dep: %.S
	-@mkdir -p obj
	$(SHELL) -ec '$(XCC) -S -M $(CFLAGS) $< | sed "s/$*.S/& $(subst $@,/,\\/)/g" > $@'

clean:
	-rm -rf obj

# Cancel .c shortcut rule
%.o: %.c

# Keep .s files around
.PRECIOUS: $(S)/bin/% obj/%.s obj/%.dep

upload: $(TARGET)
	scp $(TARGET) gumstix.cs.uwaterloo.ca:/srv/tftp/ARM/$(USER)/kern

.PHONY: all upload
