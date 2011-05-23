#ifndef TIMERS_H
#define TIMERS_H

/* Registers */
#define TIDR 0x000 // timer revision
#define TIOCP_CFG 0x010 // interface options
# define TIOCP_CFG_SOFTRESET 0x2
#define TISTAT 0x014 // timer status
# define TISTAT_RESETDONE 0x1
#define TISR 0x018 // timer interrupt status
#define TIER 0x01C // timer interrupt enable
#define TCLR 0x024 // timer control register
# define TCLR_AR 0x2 // autoreload enable
# define TCLR_ST 0x1 // timer start
#define TCRR 0x028 // this is where the time is
#define TLDR 0x02C // timer load register (loads this on overflow)
#define TOCR 0x054 // timer overflow counter (timers 1, 2 and 10 only)

/* Initialization */
void timer_init();

/* Timers */
#define TICKS_PER_MSEC 32768
unsigned long long read_timer();

#endif /* TIMERS_H */