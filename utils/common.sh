XPREFIX="arm-linux-gnueabi-"
XCC="${XPREFIX}gcc"
AS="${XPREFIX}as"
LD="${XPREFIX}ld"
AR="${XPREFIX}ar"

# Standard options
CFLAGS="-pipe -Wall -Wextra -I include -std=gnu99 -O2"

# Debug options
CFLAGS+=" -Werror"

# ARMv7 instruction set, Cortex-A8 tuning
CFLAGS+=" -march=armv7-a -mtune=cortex-a8"

# Keep frame pointers
CFLAGS+=" -fno-omit-frame-pointer -mapcs-frame -marm"

# Make assembly output more readable
CFLAGS+=" -fverbose-asm"

# No C library
CFLAGS+=" -ffreestanding"

if test -n "$DEBUG"; then
	CFLAGS+=' -g -DDEBUG -Wno-unused-function -Wno-unused-parameter'
else
	CFLAGS+=' -Wno-unused-parameter -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wstrict-prototypes -Winit-self -Wstrict-overflow -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-format-attribute -Wredundant-decls -Wnested-externs -Winline -Wvolatile-register-var -Wdisabled-optimization'
fi

CFLAGS+=" -DBUILDUSER=$USER"

# Do not link in glibc
LDFLAGS="-nostdlib"

# Custom linker script
LDFLAGS+=" -T ../omap3.ld"

# Disable demand-pageable
LDFLAGS+=" -n"

# Build id
LDFLAGS+=" --build-id=sha1"

# Link libgcc for compiler-generated function calls
LDLIBS+=" -lgcc"
