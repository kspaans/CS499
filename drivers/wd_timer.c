/* Watchdog Timer Code */
#include <machine.h>
#include <drivers/wd_timer.h>
#include <drivers/intc.h>
#include <types.h>
#include <mem.h>

static void wdt_post_wait() {
	while(mem32(WDT3_PHYS_BASE + WWPS))
		;
}

/* Start WDT3 */
void wdt_go(int value) {
	/* Reset device */
	mem32(WDT3_PHYS_BASE + WD_CFG) |= WD_CFG_SOFTRESET;
	while(!(mem32(WDT3_PHYS_BASE + WDSTAT) & WDSTAT_RESETDONE))
		;

	/* Set reload value */
	wdt_stop();
	write32(WDT3_PHYS_BASE + WLDR, value);
	wdt_post_wait();
	wdt_reload();

	/* Enable interrupts */
	mem32(WDT3_PHYS_BASE + WIER) |= WD_OVF;
	intc_register(IRQ_WDT3, NULL, 0);
	intc_set_fiq(IRQ_WDT3, 1);
	intc_intenable(IRQ_WDT3);

	/* Start counter */
	write32(WDT3_PHYS_BASE + WSPR, WSPR_START_1);
	wdt_post_wait();
	write32(WDT3_PHYS_BASE + WSPR, WSPR_START_2);
	wdt_post_wait();
}

void wdt_stop() {
	write32(WDT3_PHYS_BASE + WSPR, WSPR_STOP_1);
	wdt_post_wait();
	write32(WDT3_PHYS_BASE + WSPR, WSPR_STOP_2);
	wdt_post_wait();
}

void wdt_reload() {
	++mem32(WDT3_PHYS_BASE + WTGR);
}
