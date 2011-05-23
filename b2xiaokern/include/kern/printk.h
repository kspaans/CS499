#ifndef PRINTK_H
#define PRINTK_H

#include <stdarg.h>

/* Kernel printf */
int vprintk(const char *fmt, va_list va);
int printk(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));
void kputs(const char *str);

#endif
