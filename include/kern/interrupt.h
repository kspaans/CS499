#ifndef KERN_INTERRUPT_H
#define KERN_INTERRUPT_H

void init_cpumodes();
void init_interrupts();

typedef void (*isr_func)(int irq);

#endif /* KERN_INTERRUPT_H */
