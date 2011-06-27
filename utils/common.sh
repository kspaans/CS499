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
  XCC="$CLANG -ccc-host-triple armv7-linux-gnu -mfloat-abi=soft -integrated-as"
  XLD="arm-eabi-gcc -Wl,--plugin,/export/scratch/mspang/cross/lib/LLVMgold.so"
  XAS=${XPREFIX}gcc
  XAR="${XPREFIX}ar --plugin /export/scratch/mspang/cross/lib/LLVMgold.so"
  LLVM_AS=$(dirname $CLANG)/llvm-as
else
  XPREFIX=arm-linux-gnueabi-
  XCC=${XPREFIX}gcc
  XLD=${XPREFIX}gcc
  XAS=${XPREFIX}gcc
  XAR=${XPREFIX}ar
fi

# Standard options
CFLAGS="-pipe -Wall -Wextra -Werror -I include -std=gnu99 -O3"

# ARMv7 instruction set, Cortex-A8 tuning, ARM instrutions
CFLAGS+=" -march=armv7-a -mtune=cortex-a8 -marm"

# Keep frame pointers
CFLAGS+=" -fno-omit-frame-pointer"

# Make assembly output more readable
CFLAGS+=" -fverbose-asm"

# No C library
CFLAGS+=" -ffreestanding"

# Super ricer-mode
#CFLAGS+=" -flto"

if test -n "$DEBUG"; then
	CFLAGS+=' -g -DDEBUG -Wno-unused'
else
	CFLAGS+=' -Wno-unused-parameter -Wmissing-prototypes -Wmissing-declarations'
	CFLAGS+=' -Wold-style-definition -Wstrict-prototypes -Winit-self'
	CFLGAS+=' -Wstrict-overflow -Wfloat-equal -Wshadow -Wpointer-arith'
	CFLAGS+=' -Wcast-align -Wwrite-strings -Wmissing-format-attribute'
	CFLAGS+=' -Wredundant-decls -Wnested-externs -Wvolatile-register-var'
	CFLAGS+=' -Wdisabled-optimization'
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
