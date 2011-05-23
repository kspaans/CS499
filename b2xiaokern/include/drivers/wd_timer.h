#ifndef WD_TIMER_H
#define WD_TIMER_H

#include <types.h>

/* Registers */
#define WISR 0x018
#define WIER 0x01C
#define WCLR 0x024
#define WCCR 0x028
#define WLDR 0x02C
#define WTGR 0x030
#define WWPS 0x034
#define WSPR 0x048

#define W_PEND_WSPR_MASK 0x10
#define W_PEND_WTGR_MASK 0x08

void enable_wdt();
void disable_wdt();
void load_wdt(uint32_t time);
uint32_t read_wdt();
void kick_wdt();

#endif
