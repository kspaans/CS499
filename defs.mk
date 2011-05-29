XPREFIX = arm-linux-gnueabi-
XCC     = $(XPREFIX)gcc
AS      = $(XPREFIX)as
LD      = $(XPREFIX)ld
AR      = $(XPREFIX)ar

ECHO = /bin/echo -e

# Standard options
CFLAGS  += -pipe -Wall -I$(S)/include -std=gnu99 -O2

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

RED = \x1b[31m
BLUE = \x1b[34m
GREEN = \x1b[32m
NOC = \x1b[m
REDIT = 2>&1 1>/dev/null | sed -e "s;^;$(RED)$(TOP);" -e "s;$$;$(NOC);"

# New pattern rules
$(TARGET) : $(OBJS) $(LIBS)
	-@mkdir -p $(S)/bin
ifeq ($(suffix $(TARGET)),.a)
	@$(ECHO) "  $(GREEN)[AR] $(TOP)$(TARGET)$(NOC)"
	@$(AR) crs $@ $(OBJS) $(REDIT)
else
	@$(ECHO) "  $(GREEN)[CC] $(TOP)$(TARGET)$(NOC)"
	@$(XCC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(LDLIBS) $(REDIT)
endif

obj/%.s: %.c
	-@mkdir -p obj
	@$(ECHO) "  [CC] $(TOP)$@"
	@$(XCC) -S $(CFLAGS) -o $@ $(filter %.c,$^) $(REDIT)

obj/%.o: obj/%.s
	-@mkdir -p obj
	@$(ECHO) "  [AS] $(TOP)$@"
	@$(XCC) -c $(CFLAGS) -o $@ $< $(REDIT)

obj/%.o: %.S
	-@mkdir -p obj
	@$(ECHO) "  [AS] $(TOP)$@"
	@$(XCC) -c $(CFLAGS) -o $@ $(filter %.S,$^) $(REDIT)

# Dep generation should be silent
obj/%.dep: %.c
	-@mkdir -p obj
	@$(SHELL) -ec '$(XCC) -S -MT $(<:%.c=%.s) -M $(CFLAGS) $< | sed "s/$*.s/obj\\/$*.s obj\\/$*.dep/g" > $@'

obj/%.dep: %.S
	-@mkdir -p obj
	@$(SHELL) -ec '$(XCC) -S -M $(CFLAGS) $< | sed "s/$*.S/& obj\\/$*.dep/g" > $@'

clean:
	-@rm -rf obj
	-@rm -f $(TARGET)

# Cancel .c shortcut rule
%.o: %.c

# Keep .s files around
.PRECIOUS: $(S)/bin/% obj/%.s obj/%.dep obj/%.o

upload: $(TARGET)
	scp $(TARGET) gumstix.cs.uwaterloo.ca:/srv/tftp/ARM/$(USER)/kern

.PHONY: all upload
