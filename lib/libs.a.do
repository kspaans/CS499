DEPS="cpu.o lib.o memcpy.o printf.o string.o"
redo-ifchange ${TOP}/utils/archive ${DEPS}

${TOP}/utils/archive $3 ${DEPS}
