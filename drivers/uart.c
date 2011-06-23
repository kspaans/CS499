/* General I/O and system functions */
#include <machine.h>
#include <drivers/uart.h>
#include <types.h>
#include <mem.h>
#include <panic.h>
#include <kern/task.h>

static void uart_mode_A(void) {
	mem32(UART3_PHYS_BASE + UART_LCR_OFFSET) |= UART_DIV_EN;
}

static void uart_mode_op(void) {
	mem32(UART3_PHYS_BASE + UART_LCR_OFFSET) &= ~UART_DIV_EN;
}

static void uart_set_divisor(int div) {
	write32(UART3_PHYS_BASE + UART_DLH_OFFSET, div >> 8);
	write32(UART3_PHYS_BASE + UART_DLL_OFFSET, div & 0xff);
}

void uart_init(void) {
	/* reset device */
	write32(UART3_PHYS_BASE + UART_SYSC_OFFSET, UART_SYSC_SOFTRESET);
	while(!(mem32(UART3_PHYS_BASE + UART_SYSS_OFFSET) & UART_SYSS_RESETDONE))
		;

	/* enable enhanced features */
	write32(UART3_PHYS_BASE + UART_LCR_OFFSET, 0xBF); // mode B
	mem32(UART3_PHYS_BASE + UART_EFR_OFFSET) |= UART_EFR_ENHANCED_EN;
	write32(UART3_PHYS_BASE + UART_LCR_OFFSET, 0x00);

	uart_mode_A();
	/* set RX FIFO trigger to 1 byte, TX FIFO trigger to 16 bytes */
	write32(UART3_PHYS_BASE + UART_FCR_OFFSET, (1<<6) | (1<<4) | UART_FCR_FIFO_EN);
	write32(UART3_PHYS_BASE + UART_SCR_OFFSET, UART_SCR_RX_TRIG_GRANU1);
	/* set baud rate to 115200 (48000000/26/16 ~= 115200) */
	uart_set_divisor(26);

	uart_mode_op();
	/* configure 8 bits, no parity, 1 stop bit */
	write32(UART3_PHYS_BASE + UART_LCR_OFFSET, UART_CHARLEN_8);
	/* set final divisor to 16 and activate device */
	write32(UART3_PHYS_BASE + UART_MDR1_OFFSET, UART_MODE_UART16x);
}

int uart_intstatus(void) {
	return read32(UART3_PHYS_BASE + UART_IIR_OFFSET);
}

void uart_intenable(int interrupt) {
	mem32(UART3_PHYS_BASE + UART_IER_OFFSET) |= interrupt;
}

void uart_intdisable(int interrupt) {
	mem32(UART3_PHYS_BASE + UART_IER_OFFSET) &= ~interrupt;
}

int uart_txfull(void) {
	return mem32(UART3_PHYS_BASE + UART_SSR_OFFSET) & UART_SSR_TX_FIFO_FULL;
}

int uart_rxempty(void) {
	return !(mem32(UART3_PHYS_BASE + UART_LSR_OFFSET) & UART_DRS_MASK);
}

void uart_tx(int c) {
	write32(UART3_PHYS_BASE + UART_THR_OFFSET, c);
}

int uart_rx(void) {
	return read32(UART3_PHYS_BASE + UART_RHR_OFFSET);
}

int uart_getc(void) {
	volatile uint32_t *flags, *data;
	flags = (uint32_t *)(UART3_PHYS_BASE + UART_LSR_OFFSET);
	data = (uint32_t *)(UART3_PHYS_BASE + UART_RHR_OFFSET);

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

void uart_rx_sts_err(void) {
	volatile uint32_t *flags, *data;
	flags = (uint32_t *)(UART3_PHYS_BASE + UART_LSR_OFFSET);
	data = (uint32_t *)(UART3_PHYS_BASE + UART_RHR_OFFSET);

	if (read32(UART3_PHYS_BASE + UART_LSR_OFFSET) & UART_BI_MASK) {
		*data;
		sysrq();
		return;
	}

	panic("uart line status error");

}
