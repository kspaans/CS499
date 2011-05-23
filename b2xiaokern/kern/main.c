#include <machine.h>
#include <drivers/eth.h>
#include <drivers/intc.h>
#include <drivers/timers.h>
#include <kern/printk.h>
#include <kern/task.h>
#include <lib.h>
#include <syscall.h>

void idle() {
	/* Todo: use the ARM wait-for-interrupt instruction */
	while (1)
		Pass();
}

/* userprog.c */
void a2_init();

int main() {
	struct task *next;
	/* Start up hardware */
	init_cpumodes();
	intc_init();
	timer_init();
	eth_init(ETH1_BASE);

	/* For some reason, turning on the caches causes the kernel to hang after finishing
	   the third invocation. Maybe we have to clear the caches here. */
	//init_cache();

	/* Initialize task queues */
	init_tasks();

	/* Initialize idle task */
	int idle_tid = syscall_CreateDaemon(NULL, 7, idle);

#ifdef SUPERVISOR_TASKS
	(void)idle_tid;
#else
	 // execute idle task in system mode, so that it can sleep the processor
	get_task(idle_tid)->regs.psr |= 0x1f;
#endif

	/* Initialize first user program */
	syscall_Create(NULL, 0, a2_init);

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
