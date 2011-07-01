#include <msg.h>
#include <types.h>
#include <syscall.h>
#include <apps.h>
#include <lib.h>
#include <drivers/pmu.h>
#include <timer.h>

#define SRR_RUNS 16384

static void srrbench_child_task(void) {
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

void srrbench_task(void) {
	int tid = gettid();
	printf("srrbench_task[%d]: benchmarking SRR transaction\n", tid);

	int srrbench_chan = channel(0);

	int chans[] = { 0, 1, 2, srrbench_chan };
	printf("%d\n", sys_spawn(0, srrbench_child_task, chans, arraysize(chans), 0));

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
