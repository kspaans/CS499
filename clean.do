redo-always
find -name '*.[aso]' -exec rm {} \; 1>&2
rm -f utils/{compile,assemble,link,archive} kern/kern.elf
