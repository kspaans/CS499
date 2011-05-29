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
static void uart_isr(int irq) {
}
static void init_uart_irq() {
	intc_register(IRQ_UART3, uart_isr, 1);
}

void init_interrupts() {
	init_eth_irq();
	init_uart_irq();
}

void task_irq() {
	intc_dispatch();
}
