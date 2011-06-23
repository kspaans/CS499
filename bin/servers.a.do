PREFIX="../servers/obj/"
DEPS=(clock.o console.o net.o fs.o genesis.o)

mkdir -p ${PREFIX}
DEPS=${DEPS[@]/#/${PREFIX}/}
redo-ifchange ../utils/archive ${DEPS}

../utils/archive $3 ${DEPS}

