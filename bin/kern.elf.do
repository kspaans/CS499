PREFIX="../kern/obj/"
DEPS=(omap3.o)
LIBS="kern.a drivers.a libs.a servers.a"
LDLIBS="-lgcc"

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/link ${LIBS} ${DEPS}

../utils/link -o $3 ${DEPS} ${LIBS} ${LDLIBS}
