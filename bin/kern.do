PREFIX="../kern/obj/"
DEPS=(omap3.o)
LOCALDEPS=(ksyms.o)
LIBS="kern.a apps.a servers.a drivers.a libs.a"
LDLIBS="-lgcc"

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/link ${LIBS} ${DEPS} ${LOCALDEPS} ../omap3.ld

../utils/link -o $3 ${DEPS} ${LOCALDEPS} ${LIBS} ${LDLIBS}
