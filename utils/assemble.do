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

echo -e -n "#!/bin/bash\n${XCC} -c ${CFLAGS} \$*\n" > $3
chmod +x $3

