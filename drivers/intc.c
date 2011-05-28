#include <machine.h>
#include <drivers/intc.h>
#include <types.h>
#include <mem.h>
#include <kern/printk.h>

static struct intcps_vectors {
	volatile uint32_t itr, mir, mir_clear, mir_set, isr_set, isr_clear, pending_irq, pending_fiq;
} *intcps_vectors = (struct intcps_vectors *)(INTC_PHYS_BASE + INTCPS_VECTORS_OFFSET);

static isr_func isrs[96];

void intc_register(int slot, isr_func handler, int prio) {
	volatile int *ilr = (int *)(INTC_PHYS_BASE + INTCPS_ILR_OFFSET);
	ilr[slot] = prio << 2;
	isrs[slot] = handler;
}

void intc_intenable(int slot) {
	intcps_vectors[slot>>5].mir_clear = 1<<(slot & 31);
}

void intc_intdisable(int slot) {
	intcps_vectors[slot>>5].mir_set = 1<<(slot & 31);
}

void intc_init() {
	/* Reset module */
	write32(INTC_PHYS_BASE + INTCPS_SYSCONFIG_OFFSET, INTCPS_SOFTRESET);
	while(!(mem32(INTC_PHYS_BASE + INTCPS_SYSSTATUS_OFFSET) & INTCPS_RESETDONE))
		;

	for(int i=0; i<96; i++) {
		isrs[i] = NULL;
	}
}

void intc_reset() {
	/* Mask all interrupts */
	for(int i=0; i<3; i++) {
		intcps_vectors[i].mir = 0xffffffff;
	}
}

void intc_dispatch() {
	int irq = mem32(INTC_PHYS_BASE + INTCPS_SIR_IRQ_OFFSET) & INTCPS_ACTIVEIRQ_MASK;
	if(isrs[irq] == NULL) {
		printk("ERROR: Bad IRQ %d\n", irq);
		intcps_vectors[irq>>5].mir_set = 1<<(irq & 31);
	} else {
		isrs[irq](irq);
	}

	write32(INTC_PHYS_BASE + INTCPS_CONTROL_OFFSET, INTCPS_NEWIRQAGR);
}
