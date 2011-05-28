#include <event.h>
#include <drivers/intc.h>
#include <kern/interrupt.h>

void task_irq() {
	intc_dispatch();
}

void init_interrupts() {
	init_cpumodes();
	intc_init();
}
