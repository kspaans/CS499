XPREFIX="arm-linux-gnueabi-"
XCC="${XPREFIX}gcc"
AS="${XPREFIX}as"
LD="${XPREFIX}ld"
AR="${XPREFIX}ar"

echo -e -n "#!/bin/bash\n${AR} crs \$*" > $3
chmod +x $3

