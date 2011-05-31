#!/bin/bash

export TOP=`pwd`

redo drivers/drivers.a
redo lib/libs.a
redo servers/servers.a
redo kern/kern.elf
