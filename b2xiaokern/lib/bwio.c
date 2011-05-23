#include <drivers/uart.h>

// Blocking getchar.
int getchar() {
	return uart_getc();
}

// Blocking putchar.
void putchar(char c) {
	uart_putc(c);
}
