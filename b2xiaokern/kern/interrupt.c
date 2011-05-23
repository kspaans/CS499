#include <event.h>
#include <drivers/intc.h>
#include <kern/interrupt.h>

void task_irq() {
	intc_dispatch();
}
