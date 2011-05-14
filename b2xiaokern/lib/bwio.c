/* General I/O and system functions */
#include "machine.h"
#include "io.h"
#include "lib.h"

static void bw_putc(char c) {
	volatile int *flags, *data;
	flags = (int *)(UART3_PHYS_BASE + UART_LSR_OFFSET);
	data = (int *)(UART3_PHYS_BASE + UART_THR_OFFSET);

	while(!(*flags & UART_THRE_MASK))
		;
	*data = c;
}

/* Busy-wait I/O */
static void bw_printfunc(void *unused, const char *str, unsigned long len) {
	(void)unused;
	while(len--) {
		/* busy wait */
		if(*str == '\n') {
			bw_putc('\r');
			bw_putc('\n');
		} else {
			bw_putc(*str++);
		}
	}
}

int printf(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = func_vprintf(bw_printfunc, NULL, fmt, va);
	va_end(va);
	return ret;
}

int vprintf(const char *fmt, va_list va) {
	return func_vprintf(bw_printfunc, NULL, fmt, va);
}

void puts(const char *str) {
	printf("%s\n", str);
}

// Blocking getchar.
int getchar() {
	volatile int *flags, *data;
	flags = (int *)(UART3_PHYS_BASE + UART_LSR_OFFSET);
	data = (int *)(UART3_PHYS_BASE + UART_RBR_OFFSET);
	while(!(*flags & UART_DRS_MASK))
		;
	return *data;
}

// Blocking putchar.
void putchar(char c) {
	bw_printfunc(NULL, &c, 1);
}
