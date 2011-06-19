PREFIX="../kern/obj/"
DEPS=(omap3.o)
LOCALDEPS=(ksyms.o)
LIBS="kern.a drivers.a libs.a servers.a apps.a"
LDLIBS="-lgcc"

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/link ${LIBS} ${DEPS} ${LOCALDEPS} ../omap3.ld

../utils/link -o $3 ${DEPS} ${LOCALDEPS} ${LIBS} ${LDLIBS}
