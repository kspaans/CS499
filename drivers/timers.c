/* General I/O and system functions */
#include <machine.h>
#include <drivers/timers.h>
#include <drivers/wd_timer.h>
#include <drivers/intc.h>
#include <timer.h>
#include <types.h>
#include <mem.h>

/* GP Timer 3 is the generic high-precision counter running at 26MHz.
   GP Timer 4 is the watchdog kicker. */

volatile uint64_t gpt3_ovf_count;

static void timer3_isr(int irq) {
	++gpt3_ovf_count;
	timer_intreset(GPTIMER3);
}

static void timer4_isr(int irq) {
	wdt_reload();
	timer_intreset(GPTIMER4);
}

/* Timers */
void timers_init(void) {
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
	intc_register(IRQ_GPT3, timer3_isr, 0);
	timer_go(GPTIMER3, 0, 1);
	intc_intenable(IRQ_GPT3);

	/* Set up timer 4 to be a watchdog kicker */
	intc_register(IRQ_GPT4, timer4_isr, 0);
	// set Timer 4 to be 32KHz, same as the watchdog
	mem32(CM_PER_BASE + CM_CLKSEL_OFFSET) &= ~CM_CLKSEL_GPT4;
	timer_go(GPTIMER4, -3*WD_TICKS_PER_SEC, 1);
	intc_intenable(IRQ_GPT4);

	/* Release the watchdog */
	wdt_go(-10*WD_TICKS_PER_SEC);
}

static void timer_post_wait(int base) {
	while(mem32(base + TWPS))
		;
}

void timer_go(int base, int value, int irq) {
	// Soft-reset timer
	mem32(base + TIOCP_CFG) |= TIOCP_CFG_SOFTRESET;
	while(!(mem32(base + TISTAT) & TISTAT_RESETDONE))
		;
	// Reset counter to 0 on overflow
	write32(base + TLDR, value);
	timer_post_wait(base);
	// Trigger timer reload
	write32(base + TTGR, 0xdeadbeef);
	timer_post_wait(base);
	// Enable autoreload, start timer
	mem32(base + TCLR) |= TCLR_AR | TCLR_ST;
	timer_post_wait(base);
	if(irq) {
		mem32(base + TIER) |= TI_OVF;
		timer_post_wait(base);
	}
}

void timer_stop(int base) {
	mem32(base + TCLR) &= ~TCLR_ST;
	timer_post_wait(base);
}

void timer_intreset(int base) {
	write32(base + TISR, TI_OVF);
	timer_post_wait(base);
}

uint64_t read_timer(void) {
	return (gpt3_ovf_count << 32) + mem32(GPTIMER3 + TCRR);
}

void udelay(int usec) {
	uint64_t ticks = read_timer() + ((uint64_t)usec) * TICKS_PER_USEC;
	while(read_timer() < ticks)
		;
}
