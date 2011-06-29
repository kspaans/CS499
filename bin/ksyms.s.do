#!/bin/bash -e

redo-ifchange ../utils/gensyms kern.map

../utils/gensyms < kern.map > $3
