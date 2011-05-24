/* General I/O and system functions */
#include <machine.h>
#include <drivers/uart.h>
#include <types.h>

int uart_getc() {
	volatile uint32_t *flags, *data;
	flags = (uint32_t *)(UART3_PHYS_BASE + UART_LSR_OFFSET);
	data = (uint32_t *)(UART3_PHYS_BASE + UART_RBR_OFFSET);

	while(!(*flags & UART_DRS_MASK))
		;
	return *data;
}

void uart_putc(char c) {
	volatile uint32_t *flags, *data;
	flags = (uint32_t *)(UART3_PHYS_BASE + UART_LSR_OFFSET);
	data = (uint32_t *)(UART3_PHYS_BASE + UART_THR_OFFSET);

	while(!(*flags & UART_THRE_MASK))
		;
	*data = c;
}
