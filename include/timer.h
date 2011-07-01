#ifndef TIMER_H
#define TIMER_H

#include <types.h>

/* Timers */
// Overo uses 26MHz system clock
#define TICKS_PER_MSEC 26000ULL
#define TICKS_PER_USEC (TICKS_PER_MSEC/1000)
uint64_t read_timer(void);
void udelay(int usec);

#endif /* TIMER_H */
