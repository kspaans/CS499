XPREFIX="arm-linux-gnueabi-"
XCC="${XPREFIX}gcc"
AS="${XPREFIX}as"
LD="${XPREFIX}ld"
AR="${XPREFIX}ar"

# Standard options
CFLAGS="-pipe -Wall -I include -std=gnu99 -O2"

# ARMv7 instruction set, Cortex-A8 tuning
CFLAGS+=" -march=armv7-a -mtune=cortex-a8"

# Keep frame pointers
CFLAGS+=" -fno-omit-frame-pointer -mapcs-frame -marm"

# Make assembly output more readable
CFLAGS+=" -fverbose-asm"

# No C library
CFLAGS+=" -ffreestanding"

if [ "${DEBUG}" != "" ] ; then
    CFLAGS+= -g -DDEBUG -Wno-unused-function
fi

# Do not link in glibc
LDFLAGS="-nostdlib"

# Custom linker script
LDFLAGS+=" -T ../omap3.ld"

# Disable demand-pageable
LDFLAGS+=" -n"

# Link libgcc for compiler-generated function calls
LDLIBS+=" -lgcc"
