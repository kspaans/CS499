PREFIX="../kern/obj/"
DEPS=(omap3.o nosyms.o)
LIBS="kern.a drivers.a servers.a apps.a libs.a"
LDLIBS="-lgcc"

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/link ${LIBS} ${DEPS} ../omap3.ld

../utils/link -o $3 ${DEPS} ${LIBS} ${LDLIBS}
