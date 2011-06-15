#!/bin/bash

redo-ifchange ../utils/gensyms
redo-ifchange kern.map

../utils/gensyms < kern.map
