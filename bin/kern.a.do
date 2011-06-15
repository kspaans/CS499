PREFIX="../kern/obj/"
DEPS=(backtrace.o interrupt.o kmalloc.o main.o printk.o switch.o syscall.o task.o userprog.o)

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/archive ${DEPS}

../utils/archive $3 ${DEPS}
