#!/bin/bash -e

FILE=${1/obj\//}.S
if test ! -e "$FILE"; then
  FILE="$1.s"
fi

utils/assemble $3 ${FILE}
