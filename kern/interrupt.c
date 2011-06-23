#include <event.h>
#include <drivers/intc.h>
#include <drivers/gpio.h>
#include <kern/interrupt.h>
#include <panic.h>
#include <machine.h>
#include <mem.h>
#include <event.h>
#include <kern/task.h>

#include <drivers/timers.h>
static void clock_isr(int irq) {
	event_unblock_all(EVENT_CLOCK_TICK, 0);
	timer_intreset(GPTIMER5);
}
static void init_clock_irq(void) {
	intc_register(IRQ_GPT5, clock_isr, 0);
	timer_go(GPTIMER5, -26000, 1);
	intc_intenable(IRQ_GPT5);
}

/* In this file are the interrupts which need to be handled by tasks
	(as opposed to autonomously, as is the case with some timers). */
#include <drivers/eth.h>
static void eth_isr(int irqpin) {
	if(eth_intstatus() & ETH_INT_RSFL) {
		event_unblock_all(EVENT_ETH_RECEIVE, 0);
		eth_intreset(ETH_INT_RSFL);
		eth_intdisable(ETH_INT_RSFL);
	}
}
static void init_eth_irq(void) {
	eth_set_rxlevel(0); // interrupt if RX level > 0
	eth_intenable(ETH_INT_RSFL);
	gpio_register(GPIO_ETH1_IRQ, eth_isr, GPIOIRQ_LEVELDETECT0);
	gpio_intenable(GPIO_ETH1_IRQ);
}

#include <drivers/uart.h>
static void uart_isr(int irq) {
	switch(uart_intstatus() & UART_IIR_IT_MASK) {
	case UART_IIR_RXTO:
	case UART_IIR_RHR:
		event_unblock_all(EVENT_CONSOLE_RECEIVE, 0);
		uart_intdisable(UART_RHR_IT);
		break;
	case UART_IIR_THR:
		event_unblock_all(EVENT_CONSOLE_TRANSMIT, 0);
		uart_intdisable(UART_THR_IT);
		break;
	case UART_IIR_RX_STS_ERR:
		uart_rx_sts_err();
		break;
	default:
		panic("unhandled UART interrupt");
		break;
	}
}
static void init_uart_irq(void) {
	uart_intenable(UART_RHR_IT | UART_THR_IT | UART_LINE_STS_IT);
	intc_register(IRQ_UART3, uart_isr, 1);
	intc_intenable(IRQ_UART3);
}

void init_interrupts(void) {
	init_clock_irq();
	init_eth_irq();
	init_uart_irq();
}

void task_irq(void) {
	intc_dispatch();
}
