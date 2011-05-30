#include <event.h>
#include <drivers/intc.h>
#include <drivers/gpio.h>
#include <kern/interrupt.h>
#include <kern/printk.h>
#include <machine.h>
#include <mem.h>

/* In this file are the interrupts which need to be handled by tasks
	(as opposed to autonomously, as is the case with some timers). */
#include <drivers/eth.h>
static void eth_isr(int irqpin) {
}
static void init_eth_irq() {
	gpio_register(GPIO_ETH1_IRQ, eth_isr, GPIOIRQ_LEVELDETECT1);
}

#include <drivers/uart.h>
int udp_printf(const char *fmt, ...);
static void uart_isr(int irq) {
	udp_printf("HI intstatus %d!\n", uart_intstatus());
	switch(uart_intstatus() & UART_IIR_IT_MASK) {
	case UART_IIR_RXTO:
	case UART_IIR_RHR:
		uart_intdisable(UART_RHR_IT);
		break;
	case UART_IIR_THR:
		uart_intdisable(UART_THR_IT);
		break;
	default:
		udp_printf("Bad intstatus %d!\n", uart_intstatus());
	}
}
static void init_uart_irq() {
	intc_register(IRQ_UART3, uart_isr, 1);
	intc_intenable(IRQ_UART3);
}

void init_interrupts() {
	init_eth_irq();
	init_uart_irq();
}

void task_irq() {
	intc_dispatch();
}
