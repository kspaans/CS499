#include "io.h"
#include "lib.h"
#include "syscall.h"
#include "syscallno.h"
#include "errno.h"
#include "task.h"
#include "nameserver.h"
#include "rps.h"

void kern_syscall(int code, struct task *task) {
	int ret;
	switch(code) {
	case SYS_CREATE:
		ret = syscall_Create(task, task->regs.r0, (void *)task->regs.r1);
		break;
	case SYS_MYTID:
		ret = syscall_MyTid(task);
		break;
	case SYS_MYPTID:
		ret = syscall_MyParentTid(task);
		break;
	case SYS_PASS:
		syscall_Pass(task);
		ret = 0;
		break;
	case SYS_EXIT:
		syscall_Exit(task);
		ret = 0;
		break;
	case SYS_SEND:
		ret = syscall_Send(task, task->regs.r0, (useraddr_t)task->regs.r1, task->regs.r2, (useraddr_t)task->regs.r3, STACK_ARG(task, 5));
		break;
	case SYS_RECEIVE:
		ret = syscall_Receive(task, (useraddr_t)task->regs.r0, (useraddr_t)task->regs.r1, task->regs.r2);
		break;
	case SYS_REPLY:
		ret = syscall_Reply(task, task->regs.r0, (useraddr_t)task->regs.r1, task->regs.r2);
		break;
	default:
		ret = ERR_BADCALL;
	}
	task->regs.r0 = ret;
}

void memcpy_bench() {
	int tid = MyTid();
	printf("memcpy_bench[%d]: testing memcpy\n", tid);
	/* Test memcpy. */
	char magichands[128];
	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands+1, magichands+7, 6);
	printf("%s\n", magichands);

	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands, magichands+8, 8);
	printf("%s\n", magichands);

	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands, magichands+9, 9);
	printf("%s\n", magichands);

	sprintf(magichands, "abcdefghijklmnopqrstuvwxyz");
	memcpy(magichands, magichands+16, 7);
	printf("%s\n", magichands);
	sprintf(magichands, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abXXXXghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abc");
	memcpy(magichands, magichands+64, 64);
	printf("%s\n", magichands);

	printf("memcpy_bench[%d]: benchmarking memcpy\n", tid);
	/* Run some benchmarks! */
	char buf[1<<14];
	char buf2[1<<14];

	int i;
	unsigned long long start_time = read_timer4();
	for(i=0; i<(1<<12); i++) {
		memcpy(buf, buf2, sizeof(buf));
	}
	unsigned long long duration = read_timer4() - start_time;
	//printf("Did 64MB in %lu milliseconds\n", (unsigned long)(duration/T4_TICKS_PER_MSEC));
	//int speed = (int)((float)(1<<16) * 1000.0 * T4_TICKS_PER_MSEC / duration);
	//printf("Speed: %d kbytes per second\n", speed);
	printf("Done benchmarking memcpy, but the timers aren't configured yet :P \n");

	Exit();
}

void srr_child() {
	char buf[4];
	int i, j;
	for(i=0; i<9; i++) {
		int tid;
		int len = Receive(&tid, buf, sizeof(buf));
		printf("  Child Received, with retval %d\n", len);
		if(len < 0) {
			printf("RECEIVE FAILED: %d\n", len);
			Reply(tid, "FAILED", 6);
			continue;
		}
		if(len > sizeof(buf)) {
			printf("  Child got TRUNCATED Receive: ");
			for(j=0; j<sizeof(buf); j++)
				putchar(buf[j]);
		} else {
			printf("  Child got Receive: ");
			for(j=0; j<len; j++)
				putchar(buf[j]);
		}
		printf("\n");
		int ret = Reply(tid, "0123456789", i);
		if(ret < 0) {
			printf("  Child Reply FAILED: %d\n", ret);
		} else {
			printf("  Child Reply!\n");
		}
	}
	Exit();
}

void srr_task() {
	int tid = MyTid();
	printf("srr_task[%d]: testing SRR transaction\n", tid);
	int child = Create(1, srr_child);
	char buf[4];
	int i, j;
	for(i=0; i<10; i++) {
		int len = Send(child, "abcdefghijklmno", i, buf, sizeof(buf));
		printf(" Parent Sent, with retval %d\n", len);
		if(len < 0) {
			printf(" Parent Send failed: %d\n", len);
			continue;
		}
		if(len > sizeof(buf)) {
			printf(" Parent got TRUNCATED Reply: ");
			for(j=0; j<sizeof(buf); j++)
				putchar(buf[j]);
		} else {
			printf(" Parent got Reply: ");
			for(j=0; j<len; j++)
				putchar(buf[j]);
		}
		printf("\n");
	}
	Exit();
}

#define SRR_RUNS 16384
void srrbench_child() {
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
	Exit();
}

#define BENCH(name, code) { \
	printf("SRR Benchmarking: " name ": "); \
	start = read_timer4(); \
	for(i=0; i<SRR_RUNS; i++) code; \
	elapsed = read_timer4()-start; \
	printf("Done srr benchmarking, but the timers aren't configured yet :P \n"); \
}

	//printf("%d ms\n", (int)(elapsed/T4_TICKS_PER_MSEC)); \
	//printf("%d ns/loop\n", (int)((double)elapsed*1000000.0/T4_TICKS_PER_MSEC/SRR_RUNS)); \

void srrbench_task() {
	int tid = MyTid();
	printf("srrbench_task[%d]: benchmarking SRR transaction\n", tid);
	int child = Create(0, srrbench_child);
	char buf[512];
	int i;
	unsigned long long start, elapsed;

	BENCH("Pass", Pass())
	BENCH("4-bytes", Send(child, buf, 4, buf, 4))
	BENCH("64-bytes", Send(child, buf, 64, buf, 64))

	printf("srrbench_task[%d]: benchmark finished.\n", tid);
	Exit();
}

void task_nameservertest() {
	char namebuf[64];
	printf("Beginning test\n");
	int tid = MyTid();
	int res2 = RegisterAs("Test 1");
	ASSERT("I am myself registered", res2, 0);
	ASSERTNOERR(GetName(tid, namebuf));
	printf("Registered as %s\n", namebuf);
	printf("GetName should fail: %d\n", GetName(387, namebuf));
	int res = WhoIs("Test 1");
	ASSERT("I am myself", tid, res);
	printf("Test 1 done\n");
	int i, j;
	char str[200];
	for(i = 0; i<100; i++) {
		for(j = 0; j<60; j++) {
			str[j] = rand()%127+1;
		}
		str[60] = 0;
		res2 = RegisterAs(str);
		ASSERT("LOOP TEST Registered", res2, 0);
		res = WhoIs(str);
		ASSERT("LOOP TEST", tid, res);
	}
	printf("Test 2 done\n");
	for(i = 0; i<150; i++) {
		str[i] = 'a';
	}
	str[150] = 0;
	res = RegisterAs(str);
	ASSERT("TOO BIG TEST", res, ERR_NAMESERVER_NAMETOOLONG);

	printf("Test 3 done\n");
	for(i = 0; i<1409; i++) {
		res = RegisterAs("abc");
		ASSERT("MORE MEMORY", res, 0);
	}
	res = RegisterAs("abc");
	ASSERT("NO MORE MEMORY", res, ERR_NAMESERVER_NOMEM);
	printf("Test 4 done\n");
	Exit();
}

void a2_init() {
	ASSERTNOERR(Create(0, srrbench_task));

	ASSERTNOERR(Create(1, task_nameserver));
	ASSERTNOERR(Create(1, rps_server));


	ASSERTNOERR(Create(3, rps_rockbot));
	ASSERTNOERR(Create(3, rps_antifreqbot));
	ASSERTNOERR(Create(3, rps_freqbot));
	ASSERTNOERR(Create(3, rps_quitbot));
	ASSERTNOERR(Create(3, rps_paperbot));
	ASSERTNOERR(Create(3, rps_onebot));
	ASSERTNOERR(Create(3, rps_freqbot));
	ASSERTNOERR(Create(3, rps_randbot));
	ASSERTNOERR(Create(3, rps_antifreqbot));
	ASSERTNOERR(Create(3, rps_doublebot));

	Exit();
}

int main() {
	struct task *next;
	/* Start up hardware */
	init_timer();
	init_cache();

	/* Initialize task queues */
	init_tasks();

	/* Set an initial random seed, for repeatability */
	srand(123456789);

	/* Initialize first user program */
	syscall_Create(NULL, 0, a2_init);
	printf("It lives! Just created task.\n");

	while(1) {
		next = task_dequeue();
		if(next == NULL)
			/* No more tasks. */
			return 0;
		task_activate(next);
		task_enqueue(next);
	}
}
