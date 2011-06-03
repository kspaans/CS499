mkdir -p obj

DEPS="obj/backtrace.o obj/interrupt.o obj/kmalloc.o obj/main.o obj/omap3.o obj/printk.o obj/switch.o obj/syscall.o obj/task.o obj/userprog.o"
LIBS="../drivers/drivers.a ../lib/libs.a ../servers/servers.a"
LDLIBS="-lgcc"

redo-ifchange ${TOP}/utils/link ${DEPS}

${TOP}/utils/link -o $3 ${DEPS} ${LIBS} ${LDLIBS}
