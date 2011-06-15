#!/bin/bash

redo-always

# Builds dirs and library archives
find -type d -name 'obj' -prune -exec rm -rf {} \; 1>&2
find -type f -name '*.[a]' -exec rm -f {} \; 1>&2

# For builds that use do
find -type f -name '*.did' -exec rm -f {} \; 1>&2
rm -f .do_built
rm -rf .do_build.dir

# Aux files
rm -f utils/{compile,assemble,link,archive} bin/kern
rm -f cscope.out
