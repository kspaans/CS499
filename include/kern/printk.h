#ifndef PRINTK_H
#define PRINTK_H

#include <stdarg.h>

/* Kernel printf */
int vprintk(const char *fmt, va_list va);
int printk(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));
void panic(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)))
	__attribute__((noreturn));


void panic_pabt(void);
void panic_dabt(void);
void panic_undef(void);
void panic_unused(void);

#endif
