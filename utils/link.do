source common.sh

echo -e -n "#!/bin/bash\n${XCC} ${CFLAGS} ${LDFLAGS} \$*\n" > $3
chmod +x $3

