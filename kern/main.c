#include <machine.h>
#include <drivers/eth.h>
#include <drivers/intc.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <drivers/timers.h>
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
		Suspend();
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

	/* silly IP hack for now */
	my_ip = IP(10,0,0,my_mac.addr[5]);

	/* For some reason, turning on the caches causes the kernel to hang after finishing
	   the third invocation. Maybe we have to clear the caches here, or enable the MMU. */
	printk("Tralala before caches\r\n");
	//init_cache(); // still broken
	printk("Tralala after  caches\r\n");

	/* Initialize other interrupts */
	init_interrupts();

	/* Initialize task queues */
	init_tasks();

	/* Setup servers */
#if 0
	console_start_tasks();
	clock_start_tasks();
	net_start_tasks();
#endif

	/* Initialize idle task */
	int idle_tid = syscall_CreateDaemon(NULL, 7, idle);

#ifdef SUPERVISOR_TASKS
	(void)idle_tid;
#else
	 // execute idle task in system mode, so that it can sleep the processor
	get_task(idle_tid)->regs.psr |= 0x1f;
#endif

	/* Initialize first user program */
	syscall_Create(NULL, 2, userprog_init);

	while (nondaemon_count > 0) {
		next = task_dequeue();
		if (next == NULL)
			/* No more tasks. */
			break;
		task_activate(next);
		task_enqueue(next);
		check_stack(next);
	}
	intc_reset();
	return 0;
}
