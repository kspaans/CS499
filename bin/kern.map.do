#!/bin/bash

redo-ifchange symkern

. ../utils/common.sh

${XPREFIX}nm -n symkern
