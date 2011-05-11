#include <backtrace.h>
#include <bwio.h>
#include <omap3.h>
#include <inttypes.h>
#include <kmalloc.h>
#include <printk.h>
#include <mem.h>

void swi_handler(void) {
	void *p = kmalloc(10);
	printk("%x\r\n", p);
	bwputstr(COM3, "swi");
	for (;;);
}

void kernel_main(void) {
	bwputstr(COM3, "main\r\n");
	backtrace();
	asm("swi 0");
}
