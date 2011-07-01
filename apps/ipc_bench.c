#include <msg.h>
#include <types.h>
#include <syscall.h>
#include <apps.h>
#include <lib.h>
#include <drivers/pmu.h>
#include <timer.h>
#include <drivers/timers.h>

#define BENCH_RUNS 16384

struct counters {
	uint64_t ticks;
	uint32_t cycles;
	uint32_t p0;
	uint32_t p1;
	uint32_t p2;
	uint32_t p3;
};

static inline void bench_init(struct counters *c) {
	pmu_stop_counters();
	pmu_counter_event(0, PMU_EVTSEL_INSN_EXECUTE);
	pmu_counter_event(1, PMU_EVTSEL_INSN_ISSUED);
	pmu_counter_event(2, PMU_EVTSEL_OPERATION);
	pmu_counter_event(3, PMU_EVTSEL_EXCEPTION);
	c->cycles = pmu_cycle_counter_value();
	c->p0     = pmu_counter_value(0);
	c->p1     = pmu_counter_value(1);
	c->p2     = pmu_counter_value(2);
	c->p3     = pmu_counter_value(3);
	c->ticks  = read_timer();
	pmu_start_counters();
}

static inline void bench_done(struct counters *c) {
	pmu_stop_counters();
	c->ticks = read_timer() - c->ticks;
	c->cycles = pmu_cycle_counter_value() - c->cycles;
	c->p0 = pmu_counter_value(0) - c->p0;
	c->p1 = pmu_counter_value(1) - c->p1;
	c->p2 = pmu_counter_value(2) - c->p2;
	c->p3 = pmu_counter_value(3) - c->p3;
}

static inline void bench_print(struct counters *c) {
	printf("%lld ns, %d cycles, %d p0, %d p1, %d p2, %d p3",
	       (c->ticks * 1000000) / TICKS_PER_MSEC / BENCH_RUNS,
	       c->cycles / BENCH_RUNS,
	       c->p0 / BENCH_RUNS, c->p1 / BENCH_RUNS, c->p2 / BENCH_RUNS, c->p3 / BENCH_RUNS);
}

static inline void child_ipc_bench(int chan, int size) {
	char buf[size];
	struct iovec iov = { buf, sizeof(buf) };

	for (int i = 0; i < BENCH_RUNS; ++i) {
		xrecv(chan, &iov, 1, NULL, 0);
		xsend(chan, &iov, 1, -1, 0);
	}
}

static void ipc_bench_child_task(void) {
	for (int i = 0; i < 10; ++i)
		child_ipc_bench(3, 1 << i);
}

static inline void ipc_bench(int chan, int size) {
	char buf[size];
	struct iovec iov = { buf, sizeof(buf) };
	struct counters c;

	bench_init(&c);

	for (int i = 0; i < BENCH_RUNS; ++i) {
		xsend(chan, &iov, 1, -1, 0);
		xrecv(chan, &iov, 1, NULL, 0);
	}

	bench_done(&c);

	printf("  %d bytes, ", size);
	bench_print(&c);
	printf("\n");
}

static inline void nop_bench(void) {
	struct counters c;

	bench_init(&c);

	for (volatile int i = 0; i < BENCH_RUNS; ++i);

	bench_done(&c);

	printf("  nop, ");
	bench_print(&c);
	printf("\n");
}

static inline void yield_bench(void) {
	struct counters c;

	bench_init(&c);

	for (int i = 0; i < BENCH_RUNS; ++i)
		yield();

	bench_done(&c);

	printf("  yield, ");
	bench_print(&c);
	printf("\n");
}


void ipc_bench_task(void) {
	int chan = channel(0);

	int chans[] = { 0, 1, 2, chan };
	sys_spawn(0, ipc_bench_child_task, chans, arraysize(chans), 0);

	nop_bench();
	yield_bench();

	for (int i = 0; i < 10; ++i)
		ipc_bench(chan, 1 << i);
}
