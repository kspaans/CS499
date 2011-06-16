#!/bin/bash

redo-ifchange ../utils/gensyms kern.map

../utils/gensyms < kern.map
