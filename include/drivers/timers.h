#ifndef TIMERS_H
#define TIMERS_H

/* PRCM registers */
#define PRM_CLKSRC_CTRL_OFFSET 0x070
# define PRM_CLKSRC_CTRL_SYSCLKDIV_MASK 0xc0
# define PRM_CLKSRC_CTRL_SYSCLKDIV_DIV1 0x40 // system clock divisor = 1
# define PRM_CLKSRC_CTRL_SYSCLKDIV_DIV2 0x80 // system clock divisor = 2
#define CM_CLKSEL_OFFSET 0x040
# define CM_CLKSEL_GPT11 0x80 // in CM_CORE
# define CM_CLKSEL_GPT10 0x40 // in CM_CORE
# define CM_CLKSEL_GPT9 0x80 // in CM_PER
# define CM_CLKSEL_GPT8 0x40 // in CM_PER
# define CM_CLKSEL_GPT7 0x20 // in CM_PER
# define CM_CLKSEL_GPT6 0x10 // in CM_PER
# define CM_CLKSEL_GPT5 0x08 // in CM_PER
# define CM_CLKSEL_GPT4 0x04 // in CM_PER
# define CM_CLKSEL_GPT3 0x02 // in CM_PER
# define CM_CLKSEL_GPT2 0x01 // in CM_PER
# define CM_CLKSEL_GPT1 0x01 // in CM_WKUP

/* General-Purpose Timer Registers */
#define TIOCP_CFG 0x010 // interface options
# define TIOCP_CFG_SOFTRESET 0x2
# define TIOCP_CFG_POSTED 0x4
#define TISTAT 0x014 // timer status
# define TISTAT_RESETDONE 0x1
#define TISR 0x018 // timer interrupt status
#define TIER 0x01C // timer interrupt enable
# define TI_OVF 0x2 // overflow interrupt flag
#define TCLR 0x024 // timer control register
# define TCLR_AR 0x2 // autoreload enable
# define TCLR_ST 0x1 // timer start
#define TCRR 0x028 // this is where the time is
#define TLDR 0x02C // timer load register (loads this on overflow)
#define TTGR 0x030 // write to this register to trigger a reload
#define TWPS 0x034 // write-posted status
#define TOCR 0x054 // timer overflow counter (timers 1, 2 and 10 only)

void timers_init(void);
/* simple one-shot timer initialization: starts timer in autoreload mode */
void timer_go(int base, int value, int irq);
void timer_stop(int base);
void timer_intreset(int base);

#endif /* TIMERS_H */
