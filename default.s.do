redo-ifchange ${TOP}/utils/compile $1.c

${TOP}/utils/compile -MD -MF $3.deps.tmp -o $3 $1.c
DEPS=$(sed -e "s?^$3: ??" -e "N;" -e 's/\\//g' < $3.deps.tmp)

rm -f $3.deps.tmp
redo-ifchange ${DEPS}

