#include "io.h"
#include "lib.h"
#include "syscall.h"
#include "interrupt.h"
#include "errno.h"
#include "task.h"

void idle() {
	while (1)
		Pass()/* IDLE TODO */;
}

static void memcpy_bench() {
	int tid = MyTid();
	printk("memcpy_bench[%d]: testing memcpy\n", tid);
	/* Test memcpy. */
	char magichands[128];
	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands+1, magichands+7, 6);
	printk("%s\n", magichands);

	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands, magichands+8, 8);
	printk("%s\n", magichands);

	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands, magichands+9, 9);
	printk("%s\n", magichands);

	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands, magichands+16, 7);
	printk("%s\n", magichands);
	sprintf(magichands, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abXXXXghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abc");
	memcpy(magichands, magichands+64, 64);
	printk("%s\n", magichands);

	printk("memcpy_bench[%d]: benchmarking memcpy\n", tid);
	/* Run some benchmarks! */
	char buf[1<<14];
	char buf2[1<<14];

	int i;
	unsigned long long start_time = read_timer();
	for(i=0; i<(1<<12); i++) {
		memcpy(buf, buf2, sizeof(buf));
	}
	unsigned long long duration = read_timer() - start_time;
	printk("Did 64MB in %lu milliseconds\n", (unsigned long)(duration/TICKS_PER_MSEC));
}

static void srr_child() {
	char buf[4];
	int i, j;
	for(i=0; i<9; i++) {
		int tid;
		int len = Receive(&tid, buf, sizeof(buf));
		printk("  Child Received, with retval %d\n", len);
		if(len < 0) {
			printk("RECEIVE FAILED: %d\n", len);
			Reply(tid, "FAILED", 6);
			continue;
		}
		if(len > sizeof(buf)) {
			printk("  Child got TRUNCATED Receive: ");
			for(j=0; j<sizeof(buf); j++)
				putchar(buf[j]);
		} else {
			printk("  Child got Receive: ");
			for(j=0; j<len; j++)
				putchar(buf[j]);
		}
		printk("\n");
		int ret = Reply(tid, "0123456789", i);
		if(ret < 0) {
			printk("  Child Reply FAILED: %d\n", ret);
		} else {
			printk("  Child Reply!\n");
		}
	}
}

static void srr_task() {
	int tid = MyTid();
	printk("srr_task[%d]: testing SRR transaction\n", tid);
	int child = Create(1, srr_child);
	char buf[4];
	int i, j;
	for(i=0; i<10; i++) {
		int len = Send(child, "abcdefghijklmno", i, buf, sizeof(buf));
		printk(" Parent Sent, with retval %d\n", len);
		if(len < 0) {
			printk(" Parent Send failed: %d\n", len);
			continue;
		}
		if(len > sizeof(buf)) {
			printk(" Parent got TRUNCATED Reply: ");
			for(j=0; j<sizeof(buf); j++)
				putchar(buf[j]);
		} else {
			printk(" Parent got Reply: ");
			for(j=0; j<len; j++)
				putchar(buf[j]);
		}
		printk("\n");
	}
}

#define SRR_RUNS 16384
static void srrbench_child() {
	char buf[1024];
	int tid;
	int i;
	for(i=0; i<SRR_RUNS; i++) {
		Receive(&tid, buf, 4);
		Reply(tid, buf, 4);
	}
	for(i=0; i<SRR_RUNS; i++) {
		Receive(&tid, buf, 64);
		Reply(tid, buf, 64);
	}
}

#define BENCH(name, code) { \
	printk("SRR Benchmarking: " name ": "); \
	start = read_timer(); \
	for(i=0; i<SRR_RUNS; i++) code; \
	elapsed = read_timer()-start; \
	printk("%d ms\n", (int)(elapsed/TICKS_PER_MSEC)); \
	printk("%d ns/loop\n", (int)(elapsed*1000000/TICKS_PER_MSEC/SRR_RUNS)); \
}

static void srrbench_task() {
	int tid = MyTid();
	printk("srrbench_task[%d]: benchmarking SRR transaction\n", tid);
	int child = Create(0, srrbench_child);
	char buf[512];
	int i;
	unsigned long long start, elapsed;

	BENCH("Pass", Pass())
	BENCH("4-bytes", Send(child, buf, 4, buf, 4))
	BENCH("64-bytes", Send(child, buf, 64, buf, 64))

	printk("srrbench_task[%d]: benchmark finished.\n", tid);
}

static void a2_init() {
	ASSERTNOERR(Create(0, memcpy_bench));
}

void hello(int blah, int sts) {
	printk("hello! %d %08x\n", blah, sts);
}

int main() {
	struct task *next;
	/* Start up hardware */
	init_timer();
	init_cache();

	unsigned long long start_time = read_timer();
	volatile register int i = 1<<21;
	while(i-->0)
		;
	unsigned long long duration = read_timer() - start_time;
	printk("Did 2M iterations in %lu milliseconds\n", (unsigned long)(duration/TICKS_PER_MSEC));

	return 0;

	init_interrupts();

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
		print_task(next);
		task_activate(next);
		task_enqueue(next);
		check_stack(next);
	}
	deinit_interrupts();
	printk("Done; we're outta here!\n");
	return 0;
}
