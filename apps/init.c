/* Put your "user" programs here.
   For ease of modification, put the header files that you need
   right above your function, rather than all at the top; this allows
   functions to be easily added and removed without leaving a ton
   of #includes up top. */
#include <syscall.h>
#include <lib.h>
#include <kern/printk.h>
#include <servers/clock.h>
#include <servers/console.h>
#include <servers/fs.h>
#include <servers/genesis.h>
#include <servers/net.h>
#include <apps.h>
#include <drivers/pmu.h>

#include <lib.h>
#include <string.h>
#include <timer.h>
#include <kern/kmalloc.h>
__attribute__((unused)) static void memcpy_bench(void) {
	int tid = gettid();
	printf("memcpy_bench[%d]: benchmarking memcpy\n", tid);
	/* Run some benchmarks! */
	char *buf =  kmalloc(1<<25);
	char *buf2 = kmalloc(1<<25);

	int i;
	unsigned long long start_time = read_timer();
	for(i=0; i<(1<<2); i++) {
		memcpy(buf, buf2, 1<<25);
	}
	unsigned long long duration = read_timer() - start_time;
	printf("Did 128MB in %lu milliseconds\n", (unsigned long)(duration/TICKS_PER_MSEC));
}

#define SRR_RUNS 16384
__attribute__((unused))
static void srrbench_child(void) {
	char buf[1024];
	int i;
	struct iovec iov[] = { { buf, sizeof(buf) } };
#define BENCHSRR(n) do { iov[0].iov_len = n; for(i=0; i<SRR_RUNS; ++i) { sys_recv(3, iov, arraysize(iov), NULL, 0); sys_send(3, iov, arraysize(iov), -1, 0); } } while (0)
	BENCHSRR(0);
	BENCHSRR(1);
	BENCHSRR(2);
	BENCHSRR(4);
	BENCHSRR(8);
	BENCHSRR(16);
	BENCHSRR(32);
	BENCHSRR(64);
	BENCHSRR(128);
	BENCHSRR(256);
	BENCHSRR(512);
#undef BENCHSRR
}
#include <drivers/timers.h>
#define BENCH(name, init, code) do { \
	printf("  " name ", "); \
	pmu_cycle_counter_enable(); \
	int cstart = pmu_cycle_counter_value(); \
	start = read_timer(); \
	init; \
	for(i=0; i<SRR_RUNS; i++) code; \
	elapsed = read_timer()-start; \
	int cend = pmu_cycle_counter_value(); \
	printf("%d ns, ", (int)(elapsed*1000000/TICKS_PER_MSEC/SRR_RUNS)); \
	printf("%d cycles\n", (cend - cstart)/SRR_RUNS); \
} while(0)
__attribute__((unused)) static void srrbench_task(void) {
	int tid = gettid();
	printk("srrbench_task[%d]: benchmarking SRR transaction\n", tid);

	int srrbench_chan = channel(0);

	int chans[] = { 0, 1, 2, srrbench_chan };
	printk("%d\n", sys_spawn(0, srrbench_child, chans, arraysize(chans), 0));

	char buf[512];
	volatile int i;
	unsigned long long start, elapsed;

	struct iovec iov[] = { { buf, sizeof(buf) } };

	BENCH("nop",,);
	BENCH("yield",,yield());
#define BENCHSRR(n) BENCH(#n " bytes", iov[0].iov_len = n, ({sys_send(srrbench_chan, iov, arraysize(iov), -1, 0); sys_recv(srrbench_chan, iov, arraysize(iov), NULL, 0); }))
	BENCHSRR(0);
	BENCHSRR(1);
	BENCHSRR(2);
	BENCHSRR(4);
	BENCHSRR(8);
	BENCHSRR(16);
	BENCHSRR(32);
	BENCHSRR(64);
	BENCHSRR(128);
	BENCHSRR(256);
	BENCHSRR(512);
#undef BENCHSRR

	printf("srrbench_task[%d]: benchmark finished.\n", tid);
}

/* The first user program */
void init_task(void) {
	channel(0); /* stdin */
	channel(0); /* stdout */
	channel(0); /* fs */

	printk("console init\n");

	spawn(1, consoletx_task, SPAWN_DAEMON);
	spawn(1, consolerx_task, SPAWN_DAEMON);

	spawn(2, fileserver_task, SPAWN_DAEMON);
	spawn(1, clockserver_task, SPAWN_DAEMON);

	net_init();

	spawn(1, ethrx_task, SPAWN_DAEMON);
	spawn(1, icmpserver_task, SPAWN_DAEMON);
	spawn(1, arpserver_task, SPAWN_DAEMON);
	spawn(1, udprx_task, SPAWN_DAEMON);
	spawn(2, udpconrx_task, SPAWN_DAEMON);
	spawn(2, udpcontx_task, SPAWN_DAEMON);

	int netconin = open(ROOT_DIRFD, "/dev/netconin");
	int netconout = open(ROOT_DIRFD, "/dev/netconout");

	if(!this_host->has_uart) {
		dup(netconin, STDIN_FILENO, 0);
		dup(netconout, STDOUT_FILENO, 0);
		close(netconin);
		close(netconout);
	}

	//ASSERTNOERR(spawn(0, hashtable_test, 0));
	//ASSERTNOERR(spawn(3, memcpy_bench, 0));
	//ASSERTNOERR(spawn(4, fstest_task, 0));
	//ASSERTNOERR(spawn(2, task_reclamation_test, 0));
	//ASSERTNOERR(spawn(2, srr_task, 0));
	ASSERTNOERR(spawn(0, srrbench_task, 0));

	ASSERTNOERR(spawn(3, genesis_task, SPAWN_DAEMON));
	ASSERTNOERR(spawn(5, shell_task, 0));
	//ASSERTNOERR(spawn(6, gameoflife, 0));
}
