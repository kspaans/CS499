#include "task.h"
#include "machine.h"
#include "interrupt.h"
#include "lib.h"
#include "mem.h"

#define PRIO_HIGHEST 0x00
#define PRIO_LOWEST 0x3f

void init_cpumodes();

static struct intcps_vectors {
	volatile uint32_t itr, mir, mir_clear, mir_set, isr_set, isr_clear, pending_irq, pending_fiq;
} *intcps_vectors = (struct intcps_vectors *)(INTC_PHYS_BASE + INTCPS_VECTORS_OFFSET);

static isr_func isrs[96];

void init_vecint(int slot, isr_func handler, int prio) {
	volatile int *ilr = (int *)(INTC_PHYS_BASE + INTCPS_ILR_OFFSET);
	ilr[slot] = prio << 2;
	isrs[slot] = handler;
	intcps_vectors[slot>>5].mir_clear = 1<<(slot & 31);
}

void init_interrupts() {
	init_cpumodes();

	/* Reset module */
	write32(INTC_PHYS_BASE + INTCPS_SYSCONFIG_OFFSET, INTCPS_SOFTRESET);
	while(!(mem32(INTC_PHYS_BASE + INTCPS_SYSSTATUS_OFFSET) & INTCPS_RESETDONE))
		;

	for(int i=0; i<96; i++) {
		isrs[i] = NULL;
	}

}

void deinit_interrupts() {
	/* Mask all interrupts */
	for(int i=0; i<3; i++) {
		intcps_vectors[i].mir = 0xffffffff;
	}
}

void task_irq() {
	int irq = mem32(INTC_PHYS_BASE + INTCPS_SIR_IRQ_OFFSET) & INTCPS_ACTIVEIRQ_MASK;
	if(isrs[irq] == NULL) {
		printk("ERROR: Bad IRQ %d\n", irq);
		intcps_vectors[irq>>5].mir_set = 1<<(irq & 31);
	} else {
		isrs[irq]();
	}

	write32(INTC_PHYS_BASE + INTCPS_CONTROL_OFFSET, INTCPS_NEWIRQAGR);
}
