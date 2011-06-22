#!/bin/bash
CMD=`which redo` || CMD="./utils/do"

jobs=1

case $(hostname -f) in
	corn-syrup.csclub.uwaterloo.ca)
		jobs=10
		;;
esac

${CMD} -j${jobs} all && scp bin/kern gumstix.cs.uwaterloo.ca:/srv/tftp/ARM/${USER}/kern
