PREFIX="../lib/obj/"
DEPS=(cpu.o lib.o memcpy.o printf.o string.o)

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/archive ${DEPS}

../utils/archive $3 ${DEPS}
