/* Machine I/O functions */
#ifndef MACHINEIO_H
#define MACHINEIO_H

/* Initialization */
void init_timer();
void init_cache();

/* Timers */
#define TICKS_PER_MSEC 508
unsigned int read_timer();
#define T4_TICKS_PER_MSEC 983.04
unsigned long long read_timer4();
unsigned int read_clock();

#endif /* MACHINEIO_H */
