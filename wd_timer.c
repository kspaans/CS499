/* Watchdog Timer Code */
#include <omap3.h>
#include <inttypes.h>
#include <mem.h>
#include <wd_timer.h>

/*
 * Enable WDT2, assume we'll never care about WDT3.
 */
void enable_wdt()
{
	write32(WDT2_PHYS_BASE + WSPR, 0x0000BBBB);
	// wait for the write to clear
	while (read32(WDT2_PHYS_BASE + WWPS) & W_PEND_WSPR_MASK) {};
	write32(WDT2_PHYS_BASE + WSPR, 0x00004444);
	while (read32(WDT2_PHYS_BASE + WWPS) & W_PEND_WSPR_MASK) {};
}

/*
 * Disable WDT2, assume we'll never care about WDT3.
 */
void disable_wdt()
{
	write32(WDT2_PHYS_BASE + WSPR, 0x0000AAAA);
	// wait for the write to clear
	while (read32(WDT2_PHYS_BASE + WWPS) & W_PEND_WSPR_MASK) {};
	write32(WDT2_PHYS_BASE + WSPR, 0x00005555);
	while (read32(WDT2_PHYS_BASE + WWPS) & W_PEND_WSPR_MASK) {};
}

/*
 * Put a value in the WDT2 load register, assume we'll never care about WDT3.
 */
void load_wdt(uint32_t time)
{
	write32(WDT2_PHYS_BASE + WTGR, time);
	// wait for the write to clear
	while (read32(WDT2_PHYS_BASE + WWPS) & W_PEND_WTGR_MASK) {};
}

/*
 * Read the timer register of WDT2, assume we'll never care about WDT3.
 */
uint32_t read_wdt()
{
	return read32(WDT2_PHYS_BASE + WCCR);
}

/*
 * Force a reload of the WDT2 timer, assume we'll never care about WDT3.
 */
void kick_wdt()
{
}
