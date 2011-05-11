#ifndef PRINTK_H
#define PRINTK_H

#include <bwio.h>

#define printk(...) bwprintf(COM3, __VA_ARGS__)

#endif
