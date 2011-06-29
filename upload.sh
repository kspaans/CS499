#!/bin/bash
CMD=`which redo` || CMD="./utils/do"

jobs=1

case $(hostname) in
	corn-syrup)
		jobs=-j10
		;;
esac

${CMD} ${jobs} all && scp bin/kern gumstix.cs.uwaterloo.ca:/srv/tftp/ARM/${USER}/kern
