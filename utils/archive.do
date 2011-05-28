#!/bin/bash

source ${TOP}/utils/common.sh

echo -e -n "#!/bin/bash\n${AR} crs \$*" > $3
chmod +x $3

