#ifndef INTC_DRIVER_H
#define INTC_DRIVER_H

#include <kern/interrupt.h>

/* Registers */
#define INTCPS_REVISION_OFFSET 0x0000
#define INTCPS_SYSCONFIG_OFFSET 0x0010
# define INTCPS_SOFTRESET 0x2
# define INTCPS_AUTOIDLE 0x1
#define INTCPS_SYSSTATUS_OFFSET 0x0014
# define INTCPS_RESETDONE 0x1
#define INTCPS_SIR_IRQ_OFFSET 0x0040
# define INTCPS_ACTIVEIRQ_MASK 0x7F
#define INTCPS_SIR_FIQ_OFFSET 0x0044
#define INTCPS_CONTROL_OFFSET 0x0048
# define INTCPS_NEWFIQAGR 0x2
# define INTCPS_NEWIRQAGR 0x1
#define INTCPS_PROTECTION_OFFSET 0x004C
#define INTCPS_IDLE_OFFSET 0x0050
#define INTCPS_IRQ_PRIORITY_OFFSET 0x0060
#define INTCPS_FIQ_PRIORITY_OFFSET 0x0064
#define INTCPS_THRESHOLD_OFFSET 0x0068
#define INTCPS_VECTORS_OFFSET 0x0080
#define INTCPS_ILR_OFFSET 0x100

#define PRIO_HIGHEST 0x00
#define PRIO_LOWEST 0x3f

void intc_init();
void intc_reset();

void intc_register(int slot, isr_func handler, int prio);
void intc_intenable(int slot);
void intc_intdisable(int slot);
void intc_dispatch();

#endif /* INTC_DRIVER_H */
