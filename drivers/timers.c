/* General I/O and system functions */
#include <machine.h>
#include <drivers/timers.h>
#include <timer.h>
#include <types.h>
#include <mem.h>

/* incremented by timer3_isr in kern/interrupt.c */
volatile unsigned long long gpt3_ovf_count;

/* Timers */
void timer_init() {
	/* Make the system clock predictable */
	uint32_t reg = mem32(PRM_GLOBAL_BASE + PRM_CLKSRC_CTRL_OFFSET);
	reg &= ~PRM_CLKSRC_CTRL_SYSCLKDIV_MASK;
	reg |= PRM_CLKSRC_CTRL_SYSCLKDIV_DIV1;
	write32(PRM_GLOBAL_BASE + PRM_CLKSRC_CTRL_OFFSET, reg);

	/* Default all clocks to 26MHz (SYS_CLK) */
	mem32(CM_WKUP_BASE + CM_CLKSEL_OFFSET) |= CM_CLKSEL_GPT1;
	mem32(CM_PER_BASE + CM_CLKSEL_OFFSET) |= 0xff; /* GPT2 through GPT9 */
	mem32(CM_CORE_BASE + CM_CLKSEL_OFFSET) |= 0xc0; /* GPT10 and GPT11 */

	/* Set up timer 3 to be a high-precision counter */
	gpt3_ovf_count = 0;
	mem32(GPTIMER3 + TIOCP_CFG) |= TIOCP_CFG_SOFTRESET;
	while(!(mem32(GPTIMER3 + TISTAT) & TISTAT_RESETDONE))
		;
	// Disable write-posting
	mem32(GPTIMER3 + TIOCP_CFG) &= ~TIOCP_CFG_POSTED;
	// Reset counter to 0 on overflow
	write32(GPTIMER3 + TLDR, 0);
	// Trigger timer reload
	write32(GPTIMER3 + TTGR, 0xdeadbeef);
	// Enable autoreload, start timer
	mem32(GPTIMER3 + TCLR) |= TCLR_AR | TCLR_ST;
}

unsigned long long read_timer() {
	return (gpt3_ovf_count << 32) + mem32(GPTIMER3 + TCRR);
}

void udelay(int usec) {
	unsigned long long ticks = read_timer() + ((unsigned long long)usec) * TICKS_PER_USEC;
	while(read_timer() < ticks)
		;
}
