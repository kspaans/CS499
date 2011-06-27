top=$utils/..
useclang=$top/bin/use-clang

if test -e "$useclang"; then
  if test -z "$CLANG"; then
    echo >&2 "set \$CLANG to your clang"
    exit 1
  fi
  redo-ifchange $useclang
else
  redo-ifcreate $useclang
fi

if test -e "$useclang"; then
  XPREFIX=arm-linux-gnueabi-
  XCC="$CLANG -ccc-host-triple armv7-linux-gnu -mfloat-abi=soft"
  XLD=${XPREFIX}gcc
  XAS=${XPREFIX}gcc
  XAR=${XPREFIX}ar
else
  XPREFIX=arm-linux-gnueabi-
  XCC=${XPREFIX}gcc
  XLD=${XPREFIX}gcc
  XAS=${XPREFIX}gcc
  XAR=${XPREFIX}ar
fi

# Standard options
CFLAGS="-pipe -Wall -Wextra -I include -std=gnu99 -O3"

# Debug options
CFLAGS+=" -Werror"

# ARMv7 instruction set, Cortex-A8 tuning
CFLAGS+=" -march=armv7-a -mtune=cortex-a8"

# Keep frame pointers
CFLAGS+=" -fno-omit-frame-pointer -marm"

# Make assembly output more readable
CFLAGS+=" -fverbose-asm"

# No C library
CFLAGS+=" -ffreestanding"

if test -n "$DEBUG"; then
	CFLAGS+=' -g -DDEBUG -Wno-unused-function -Wno-unused-parameter'
else
	CFLAGS+=' -Wno-unused-parameter -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wstrict-prototypes -Winit-self -Wstrict-overflow -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-format-attribute -Wredundant-decls -Wnested-externs -Wvolatile-register-var -Wdisabled-optimization'
fi

CFLAGS+=" -DBUILDUSER=$USER"

# Do not link in glibc
LDFLAGS="-nostdlib"

# Disable demand-pageable
LDFLAGS+=" -n"

# Build id
LDFLAGS+=" -Wl,--build-id=sha1"

# Link libgcc for compiler-generated function calls
LDLIBS+=" -lgcc"
