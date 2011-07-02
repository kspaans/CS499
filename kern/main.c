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
#include <drivers/pmu.h>
#include <lib.h>
#include <syscall.h>
#include <ip.h>
#include <mem.h>
#include <panic.h>
#include <drivers/etm.h>

#include <servers/clock.h>
#include <servers/console.h>
#include <servers/net.h>
#include <apps.h>

static void idle_task(void) {
	for (;;)
		sys_suspend();
}

int main(void) {
	struct task *next;

	/* Set the CPU speed */
	uint32_t skuid = read32(DEVICEID_BASE + DEVICEID_SKUID_OFFSET);
	uint32_t cpuspeed_id = skuid & DEVICEID_SKUID_CPUSPEED_MASK;
	uint32_t clksel_val = (1<<19) | 12;
	if(cpuspeed_id == DEVICEID_SKUID_CPUSPEED_720)
		clksel_val |= (720 << 8);
	else if(cpuspeed_id == DEVICEID_SKUID_CPUSPEED_600)
		clksel_val |= (600 << 8);
	else
		panic("Unsupported CPU!");
	write32(CM_MPU_BASE + PRM_CLKSEL1_PLL_MPU_OFFSET, clksel_val);

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
	syscall_spawn(NULL, 7, idle_task, NULL, 0, SPAWN_DAEMON);

	pmu_enable();
	trace_init();

	printk("userspace init\n");

	/* Initialize first user program */
	syscall_spawn(NULL, 6, init_task, NULL, 0, 0);

	while (nondaemon_count > 0) {
		next = schedule();
		task_activate(next);
		check_stack(next);
	}

	pmu_disable();
	intc_reset();
	eth_deinit();
	deinit_mmu();
	return 0;
}
