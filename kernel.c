#include <backtrace.h>
#include <bwio.h>
#include <omap3.h>
#include <inttypes.h>
#include <kmalloc.h>
#include <string.h>
#include <printk.h>
#include <mem.h>
#include <leds.h>

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
			return;
		}
	}
	//asm("swi 0");
}
