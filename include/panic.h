#ifndef PANIC_H
#define PANIC_H

void panic(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)))
	__attribute__((noreturn));

void panic_pabt(void);
void panic_dabt(void);
void panic_undef(void);
void panic_unused(void);

#endif
