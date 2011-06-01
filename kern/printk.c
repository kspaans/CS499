/* General I/O and system functions */
#include <drivers/uart.h>
#include <lib.h>

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

int vprintk(const char *fmt, va_list va) {
	return func_vprintf(bw_printfunc, NULL, fmt, va);
}
