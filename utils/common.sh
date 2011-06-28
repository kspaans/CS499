top=$utils/..
useclang=$top/bin/use-clang

if test -e "$useclang"; then
  redo-ifchange $useclang
else
  redo-ifcreate $useclang
fi

XPREFIX=arm-eabi-
XCC=${XPREFIX}gcc
XLD=${XPREFIX}gcc
XAS=${XPREFIX}gcc
XAR=${XPREFIX}ar

if test -e "$useclang"; then
  # comment for old-style compile
  #emit_llvm=-emit-llvm

  LLPREFIX=/export/scratch/mspang/cross
  LLPLUGIN=$LLPREFIX/lib/LLVMgold.so
  XCC="$LLPREFIX/bin/clang
	-ccc-host-triple armv7-linux-gnu
	-mfloat-abi=soft
	$emit_llvm"
  XLD="$LLPREFIX/bin/arm-eabi-gcc
	-Wl,--plugin,$LLPLUGIN"
  XAR="$LLPREFIX/bin/arm-eabi-ar
	--plugin $LLPLUGIN"
  LAS="$LLPREFIX/bin/llvm-as"
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
CFLAGS+=" -flto"

if test -n "$DEBUG"; then
	CFLAGS+=' -g -DDEBUG -Wno-unused'
else
	CFLAGS+=' -Wno-unused-parameter -Wmissing-prototypes -Wmissing-declarations'
	CFLAGS+=' -Wold-style-definition -Wstrict-prototypes -Winit-self'
	CFLAGS+=' -Wstrict-overflow -Wfloat-equal -Wshadow -Wpointer-arith'
	CFLAGS+=' -Wcast-align -Wwrite-strings -Wmissing-format-attribute'
	CFLAGS+=' -Wredundant-decls -Wnested-externs -Wvolatile-register-var'
	CFLAGS+=' -Wdisabled-optimization'
fi

CFLAGS+=" -DBUILDUSER=$USER"

# Do not link in glibc
LDFLAGS="-nostdlib"

# Disable demand-pageable
LDFLAGS+=" -n"

# Link libgcc for compiler-generated function calls
LDLIBS+=" -lgcc"
