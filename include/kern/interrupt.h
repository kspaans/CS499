#ifndef KERN_INTERRUPT_H
#define KERN_INTERRUPT_H

void init_cpumodes();

typedef void (*isr_func)();

#endif /* KERN_INTERRUPT_H */
