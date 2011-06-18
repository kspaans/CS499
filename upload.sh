#!/bin/bash
CMD=`which redo` || CMD="./utils/do"
${CMD} all -j10 && scp bin/kern gumstix.cs.uwaterloo.ca:/srv/tftp/ARM/${USER}/kern
