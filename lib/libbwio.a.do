DEPS="cpu.o lib.o libs.o printf.o ip.o string.o bwio.o"
redo-ifchange ${TOP}/utils/archive ${DEPS}

${TOP}/utils/archive $3 ${DEPS}
