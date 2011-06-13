#!/bin/bash

export TOP=`pwd`

redo bin/drivers.a
redo bin/libs.a
redo bin/servers.a
redo bin/kern.elf
