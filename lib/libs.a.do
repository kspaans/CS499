mkdir -p obj

DEPS="obj/cpu.o obj/lib.o obj/memcpy.o obj/printf.o obj/string.o"
redo-ifchange ${TOP}/utils/archive ${DEPS}

${TOP}/utils/archive $3 ${DEPS}
