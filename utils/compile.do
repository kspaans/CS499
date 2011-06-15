#!/bin/bash

redo-ifchange common.sh
source common.sh

echo -e -n "#!/bin/bash\n${XCC} -S ${CFLAGS} \$*\n" > $3
chmod +x $3

