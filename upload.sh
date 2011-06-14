#!/bin/bash
CMD=`which redo` || CMD="./utils/do"
${CMD} all && scp bin/kern.elf gumstix.cs.uwaterloo.ca:/srv/tftp/ARM/${USER}/kern
