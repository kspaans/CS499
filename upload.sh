#!/bin/bash
CMD=`which redo` || CMD="./utils/do"

jobs=1

case $(hostname) in
	corn-syrup)
		jobs=10
		;;
esac

${CMD} -j${jobs} all && scp bin/kern gumstix.cs.uwaterloo.ca:/srv/tftp/ARM/${USER}/kern
