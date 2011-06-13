PREFIX="../kern/obj/"
DEPS=(backtrace.o interrupt.o kmalloc.o main.o omap3.o printk.o switch.o syscall.o task.o userprog.o)
LIBS="drivers.a libs.a servers.a"
LDLIBS="-lgcc"

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ${TOP}/utils/link ${DEPS}

${TOP}/utils/link -o $3 ${DEPS} ${LIBS} ${LDLIBS}
