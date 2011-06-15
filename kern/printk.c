/* General I/O and system functions */
#include <drivers/uart.h>
#include <lib.h>

__attribute__((noreturn))
void prm_reset(void);

/* Busy-wait I/O */
static void bw_printfunc(void *unused, const char *str, size_t len) {
	(void)unused;
	while(len--) {
		/* busy wait */
		if(*str == '\n') {
			uart_putc('\r');
			uart_putc('\n');
		} else {
			uart_putc(*str++);
		}
	}
}

int printk(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = func_vprintf(bw_printfunc, NULL, fmt, va);
	va_end(va);
	return ret;
}

void panic(const char *fmt, ...) {
	printk("panic: ");

	va_list va;
	va_start(va, fmt);
	func_vprintf(bw_printfunc, NULL, fmt, va);
	va_end(va);

	printk("\n");

	/* hacks */
	for (volatile int i = 0; i < 1000000; ++i);

	prm_reset();
}

int vprintk(const char *fmt, va_list va) {
	return func_vprintf(bw_printfunc, NULL, fmt, va);
}
