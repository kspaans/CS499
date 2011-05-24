/* General I/O and system functions */
#include <machine.h>
#include <drivers/timers.h>
#include <types.h>

/* Timers */
#define TIMER_INITIAL 0xFFFFFFFF
void timer_init() {
	volatile uint32_t *cfg, *status, *load, *clr;
	cfg = (uint32_t *)(GPTIMER1 + TIOCP_CFG);
	status = (uint32_t *)(GPTIMER1 + TISTAT);
	load = (uint32_t *)(GPTIMER1 + TLDR);
	clr = (uint32_t *)(GPTIMER1 + TCLR);

	*cfg |= TIOCP_CFG_SOFTRESET;
	while(!(*status & TISTAT_RESETDONE))
		;
	*load = 0;
	*clr |= TCLR_AR | TCLR_ST;
}

unsigned long long read_timer() {
	volatile uint32_t *crr, *ocr;
	crr = (uint32_t *)(GPTIMER1 + TCRR);
	ocr = (uint32_t *)(GPTIMER1 + TOCR);

	return ((unsigned long long)(*ocr & 0x00FFFFFF) << 32) + (unsigned long)(*crr);
}
