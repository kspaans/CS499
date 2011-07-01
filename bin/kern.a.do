#!/bin/bash -e

DEPS=(kern/backtrace.c kern/interrupt.c kern/kmalloc.c
      kern/main.c kern/printk.c kern/switch.S kern/syscall.S
      kern/task.c kern/ksyms.c kern/info.c kern/mmu.c kern/cpu.S)

../utils/archive $3 ${DEPS[@]/#/../}
