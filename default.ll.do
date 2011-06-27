#!/bin/bash -e

FILE=${1/obj\//}.c

utils/compile $3 ${FILE}
