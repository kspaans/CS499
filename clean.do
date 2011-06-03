#!/bin/bash

redo-always
find -type d -name 'obj' -prune -exec rm -rf {} \; 1>&2
find -type f -name '*.[a]' -exec rm -f {} \; 1>&2
find -type f -name '*.did' -exec rm -f {} \; 1>&2
rm -f utils/{compile,assemble,link,archive} kern/kern.elf
rm -f cscope.out
