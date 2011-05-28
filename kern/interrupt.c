#include <event.h>
#include <drivers/intc.h>
#include <drivers/gpio.h>
#include <kern/interrupt.h>
#include <kern/printk.h>
#include <machine.h>
#include <mem.h>

#include <drivers/eth.h>
static void eth_isr(int irqpin) {
}
static void init_eth_irq() {
}

#include <drivers/uart.h>
static void uart_isr(int irq) {
}
static void init_uart_irq() {
}

#include <drivers/timers.h>
extern volatile unsigned long long gpt3_ovf_count;
static void timer3_isr(int irq) {
	++gpt3_ovf_count;
	// clear interrupt
	write32(GPTIMER3 + TISR, TI_OVF);
}
static void init_timers_irq() {
	intc_register(IRQ_GPT3, timer3_isr, 0);
	// Enable overflow interrupt
	mem32(GPTIMER3 + TIER) |= TI_OVF;
	intc_intenable(IRQ_GPT3);
}

void init_interrupts() {
	init_cpumodes();
	intc_init();
	gpio_init();

	init_timers_irq();
}

void task_irq() {
	intc_dispatch();
}
