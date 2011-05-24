#ifndef KMALLOC_H
#define KMALLOC_H

#include <types.h>

#define KMALLOC_ALIGN 8

void __attribute__((malloc)) *kmalloc(size_t size);

#endif
