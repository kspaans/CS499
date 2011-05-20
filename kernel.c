#include <backtrace.h>
#include <bwio.h>
#include <omap3.h>
#include <inttypes.h>
#include <kmalloc.h>
#include <string.h>
#include <printk.h>
#include <mem.h>
#include <leds.h>
#include <wd_timer.h>

void swi_handler(void) {
	void *p = kmalloc(10);
	printk("%p\n", p);
	bwputstr(COM3, "swi");

	for (;;);
}

void kernel_main(void) {
	bwputstr(COM3, "main\n");
	backtrace();
	for(;;) {
		for(enum leds led = LED1; led <= LED5; led++) {
			led_set(led, 1);
			for(volatile int i=0; i<200000; i++)
				;
			led_set(led, 0);
		}
		if(*(volatile int *)(UART3_PHYS_BASE + UART_LSR_OFFSET) & UART_DRS_MASK) {
			bwgetc(COM3);
			break;//return;
		}
	}
	bwprintf(COM3, "The WD_SYSCONFIG status register looks like %x\r\n", read32(WDT2_PHYS_BASE + 0x10));
	bwprintf(COM3, "The WIER status register looks like %x.\r\n", read32(0x4831401C));
	//write32(0x4831401C, 0x1);
	bwprintf(COM3, "The WIER status register's now like %x.\r\n", read32(0x4831401C));
	bwprintf(COM3, "The time register is currently %x\r\n", read_wdt());
	//load_wdt(0xFFFF0ACE); // about 4s with default settings
	bwprintf(COM3, "The time register is now %x\r\n", read_wdt());
	enable_wdt();
	for (;;) {
		for(enum leds led = LED1; led <= LED5; led++) {
			led_set(led, 1);
			for(volatile int i=0; i<200000; i++)
				;
			led_set(led, 0);
		}
		bwprintf(COM3, "Timer register is %x\r\n", read_wdt());
		//if (read32(0x48314028) == 0xFFFFFFFF) {
		bwprintf(COM3, "PRM_RSTST register is %x\r\n", read32(0x48307258));
		//bwprintf(COM3, "PRM_RSTST register is %x\r\n", read32(0x48307258));}
		if (bwgetc(COM3) == 'q') break;
	}

	//asm("swi 0");
}
