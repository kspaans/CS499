DEPS="backtrace.o interrupt.o kmalloc.o main.o omap3.o printk.o switch.o syscall.o task.o userprog.o clock.o console.o"
LIBS="../drivers/drivers.a ../lib/libbwio.a"
LDLIBS="-lgcc"

redo-ifchange ${TOP}/utils/link ${DEPS}

${TOP}/utils/link -o $3 ${DEPS} ${LIBS} ${LDLIBS}
