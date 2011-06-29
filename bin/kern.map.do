#!/bin/bash -e

redo-ifchange symkern

. ../utils/common.sh

${XPREFIX}nm -n symkern > $3
