#!/bin/bash -e

DEPS=(kern/obj/omap3.o lib/obj/compat.o kern/obj/nosyms.o
      bin/kern.a bin/apps.a
      bin/servers.a bin/drivers.a
      bin/libs.a)

mkdir -p ../kern/obj

../utils/link $3 $1.link ${DEPS[@]/#/../}
