#include <machine.h>
#include <drivers/eth.h>
#include <drivers/intc.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <drivers/timers.h>
#include <drivers/cpu.h>
#include <drivers/mmu.h>
#include <kern/printk.h>
#include <kern/task.h>
#include <lib.h>
#include <syscall.h>
#include <ip.h>

#include <servers/clock.h>
#include <servers/console.h>
#include <servers/net.h>

void idle() {
	for (;;)
		sys_suspend();
}

/* userprog.c */
void userprog_init();

int main() {
	struct task *next;

	/* Basic hardware initialization */
	init_cpumodes(); // set up CPU modes for interrupt handling
	intc_init(); // initialize interrupt controller
	gpio_init(); // initialize gpio interrupt system

	/* Start up hardware */
	timers_init(); // must come first, since it initializes the watchdog
	eth_init();
	uart_init();

	/* For some reason, turning on the caches causes the kernel to hang after finishing
	   the third invocation. Maybe we have to clear the caches here, or enable the MMU. */
	printk("mmu init\n");
	prep_pagetable();
	init_mmu();

	printk("cache init\n");
	init_cache();

	/* Initialize other interrupts */
	init_interrupts();

	/* Initialize task queues */
	init_tasks();

	/* Initialize idle task */
	syscall_create(NULL, 7, idle, CREATE_DAEMON);

	cpu_info();

	printk("userspace init\n");

	/* Initialize first user program */
	syscall_create(NULL, 6, userprog_init, 0);

	while (nondaemon_count > 0) {
		next = schedule();
		task_activate(next);
		check_stack(next);
	}
	intc_reset();
	eth_deinit();
	deinit_mmu();
	return 0;
}
