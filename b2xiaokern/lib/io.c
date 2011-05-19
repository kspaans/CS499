/* General I/O and system functions */
#include "machine.h"
#include "io.h"

/* Timers */
#define TIMER_INITIAL 0xFFFFFFFF
void init_timer() {
	volatile int *cfg, *status, *load, *clr;
	cfg = (int *)(GPTIMER1 + TIOCP_CFG);
	status = (int *)(GPTIMER1 + TISTAT);
	load = (int *)(GPTIMER1 + TLDR);
	clr = (int *)(GPTIMER1 + TCLR);
	*cfg |= TIOCP_CFG_SOFTRESET;
	while(!(*status & TISTAT_RESETDONE))
		;
	*load = 0;
	*clr |= TCLR_AR | TCLR_ST;
}

unsigned long long read_timer() {
	volatile int *crr, *ocr;
	crr = (int *)(GPTIMER1 + TCRR);
	ocr = (int *)(GPTIMER1 + TOCR);
	return ((unsigned long long)(*ocr & 0x00FFFFFF) << 32) + (unsigned long)(*crr);
}
