DEPS="backtrace.o interrupt.o kmalloc.o main.o omap3.o printk.o switch.o syscall.o task.o userprog.o"
LIBS="../drivers/drivers.a ../lib/libs.a ../servers/servers.a"
LDLIBS="-lgcc"

redo-ifchange ${TOP}/utils/link ${DEPS}

${TOP}/utils/link -o $3 ${DEPS} ${LIBS} ${LDLIBS}
