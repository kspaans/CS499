#!/bin/bash -e

cd ..

DEPS="kern/obj/omap3.o kern/obj/nosyms.o
      bin/kern.a bin/apps.a bin/servers.a bin/drivers.a bin/libs.a"
LDLIBS="-lgcc"

mkdir -p kern/obj

redo-ifchange ${DEPS}
utils/link bin/$3 ${DEPS} ${LDLIBS}
