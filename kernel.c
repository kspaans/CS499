#include <bwio.h>
#include <omap3.h>
#include <inttypes.h>
#include <mem.h>

void swi_handler(void) {
	bwputstr(COM3, "swi");
	for (;;);
}

void kernel_main(void) {
	bwputstr(COM3, "main\r\n");
	asm("swi 0");
}
