#!/bin/bash -e

redo-ifchange kern
redo-always
size kern >&2
