XPREFIX="arm-linux-gnueabi-"
XCC="${XPREFIX}gcc"
AS="${XPREFIX}as"
LD="${XPREFIX}ld"
AR="${XPREFIX}ar"

# Standard options
CFLAGS="-pipe -Wall -I${TOP}/include -std=gnu99 -O2 -s"

# ARMv7 instruction set, Cortex-A8 tuning
CFLAGS+=" -march=armv7-a -mtune=cortex-a8"

# Keep frame pointers
CFLAGS+=" -fno-omit-frame-pointer -mapcs-frame -marm"

# Make assembly output more readable
CFLAGS+=" -fverbose-asm"

# No unqualified builtin functions
CFLAGS+=" -fno-builtin"

# Supervisor-mode tasks
CFLAGS+=" -DSUPERVISOR_TASKS"

if [ "${DEBUG}" != "" ] ; then
    CFLAGS+= -g -DDEBUG -Wno-unused-function
fi

# Do not link in glibc
LDFLAGS="-nostdlib -s"

# Custom linker script
LDFLAGS+=" -T ${TOP}/omap3.ld"

# Disable demand-pageable
LDFLAGS+=" -n"

# Link libgcc for compiler-generated function calls
LDLIBS+=" -lgcc"

