mkdir -p obj

DEPS="obj/clock.o obj/console.o obj/net.o"
redo-ifchange ${TOP}/utils/archive ${DEPS}

${TOP}/utils/archive $3 ${DEPS}

