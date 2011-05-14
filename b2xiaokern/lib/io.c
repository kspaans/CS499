/* General I/O and system functions */
#include "machine.h"
#include "io.h"

/* Timers */
#define TIMER_INITIAL 0xFFFFFFFF
void init_timer() {
	/* TODO */
}

unsigned long long read_timer4() {
	return 0x42424242;
	/* TODO */
}

unsigned int read_timer() {
	return 0x42424242;
	/* TODO */
}

unsigned int read_clock() {
	return 0x42424242;
	/* TODO */
}
