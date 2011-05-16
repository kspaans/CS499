/* Machine I/O functions */
#ifndef MACHINEIO_H
#define MACHINEIO_H

/* Initialization */
void init_timer();
void init_cache();

/* Timers */
#define TICKS_PER_MSEC 32768
unsigned long long read_timer();

#endif /* MACHINEIO_H */
