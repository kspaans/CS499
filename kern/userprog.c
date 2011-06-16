/* Put your "user" programs here.
   For ease of modification, put the header files that you need
   right above your function, rather than all at the top; this allows
   functions to be easily added and removed without leaving a ton
   of #includes up top. */
#include <syscall.h>
#include <lib.h>
#include <kern/printk.h>
#include <servers/fs.h>

void consoletx_task();
void consolerx_task();
void clockserver_task();
void ethrx_task();
void icmpserver_task();
void arpserver_task();
void udprx_task();
void udpconrx_task();
void fileserver_task();

#include <eth.h>
#include <servers/net.h>
__attribute__((unused)) static void udp_console_loop() {
	printf("Type characters to send to the remote host; Ctrl+D to quit\n");

	udp_printf("Hello from ");
	for(int i=0; i<5; i++) {
		udp_printf("%02x:", my_mac.addr[i]);
	}
	udp_printf("%02x\n", my_mac.addr[5]);

	for(;;) {
		char c = getchar();
		if(c == 4)
			return;
		if(c == '\r')
			c = '\n';
		udp_printf("%c", c);
	}
}

#include <drivers/leds.h>
#include <servers/clock.h>
__attribute__((unused)) static void flash_leds() {
	printf("Now flashing the blinkenlights.\n");
	for(;;) {
		for(enum leds led = LED1; led <= LED5; led++) {
			led_set(led, 1);
			msleep(100);
			led_set(led, 0);
		}
	}
}

#include <machine.h>
#include <drivers/uart.h>
__attribute__((unused)) static void console_loop() {
	volatile uint32_t *flags, *data;
	flags = (uint32_t *)(UART3_PHYS_BASE + UART_LSR_OFFSET);
	data = (uint32_t *)(UART3_PHYS_BASE + UART_RHR_OFFSET);

	printf("Press q to quit.\n");
	for(;;) {
		if(*flags & UART_DRS_MASK) {
			char c = getchar();
			if(c == 'q')
				return;
		}
		Pass();
	}
}

#include <kern/backtrace.h>
#include <lib.h>
#include <string.h>
#include <timer.h>
__attribute__((unused)) static void memcpy_bench() {
	backtrace();
	int tid = MyTid();
	printf("memcpy_bench[%d]: benchmarking memcpy\n", tid);
	/* Run some benchmarks! */
	char buf[1<<14];
	char buf2[1<<14];

	int i;
	unsigned long long start_time = read_timer();
	for(i=0; i<(1<<8); i++) {
		memcpy(buf, buf2, sizeof(buf));
	}
	unsigned long long duration = read_timer() - start_time;
	printf("Did 4MB in %lu milliseconds\n", (unsigned long)(duration/TICKS_PER_MSEC));
}

#define SRR_RUNS 16384
int srrbench_chan;
__attribute__((unused))
static void srrbench_child() {
	char buf[1024];
	int tid;
	int i;
	for(i=0; i<SRR_RUNS; i++) {
		MsgReceive(srrbench_chan, &tid, NULL, buf, 4);
		MsgReply(tid, 4, buf, 4, -1);
	}
	for(i=0; i<SRR_RUNS; i++) {
		MsgReceive(srrbench_chan, &tid, NULL, buf, 64);
		MsgReply(tid, 64, buf, 64, -1);
	}
}
#include <drivers/timers.h>
#define BENCH(name, code) { \
	printf("SRR Benchmarking: " name ": "); \
	start = read_timer(); \
	for(i=0; i<SRR_RUNS; i++) code; \
	elapsed = read_timer()-start; \
	printf("%d ms\n", (int)(elapsed/TICKS_PER_MSEC)); \
	printf("%d ns/loop\n", (int)(elapsed*1000000/TICKS_PER_MSEC/SRR_RUNS)); \
}
__attribute__((unused)) static void srrbench_task() {
	int tid = MyTid();
	printf("srrbench_task[%d]: benchmarking SRR transaction\n", tid);
	//int child = Create(0, srrbench_child);
	char buf[512];
	int i;
	unsigned long long start, elapsed;

	srrbench_chan = ChannelOpen();

	BENCH("Pass", Pass())
	BENCH("4-bytes", MsgSend(srrbench_chan, 0, buf, 4, buf, 4, NULL))
	BENCH("64-bytes", MsgSend(srrbench_chan, 0, buf, 64, buf, 64, NULL))

	printf("srrbench_task[%d]: benchmark finished.\n", tid);
}

static void nulltask() {
}
static void task_reclamation_2() {
	for(int i=0; i<1000; ++i) {
		printk("%05x ", Create(5, nulltask));
		printk("%05x\n", Create(5, nulltask));
	}
}
__attribute__((unused)) static void task_reclamation_test() {
	for(int i=0; i<1000; ++i) {
		printk("%05x ", Create(1, nulltask));
		printk("%05x ", Create(1, nulltask));
		printk("%05x\n", Create(3, nulltask));
	}
	Create(4, task_reclamation_2);
}

#if 0
int advsrr_chan1;
#define TESTRECV(i,buf,bufsz) { \
	msglen = MsgReceive(advsrr_chan1, &tid, &msgcode, buf, bufsz); \
	printf("Receive #%d (child %d): %d", i, msgcode, msglen); \
	if(!buf) printf(" (null) [] "); \
	else if(msglen > bufsz) printf(" (truncated) [%.*s] ", bufsz, buf); \
	else printf(" [%.*s] ", msglen, buf); \
	/* msglen = MsgRead(tid, msgbuf, 0, 32); \
	printf(" read %d [%.*s] ", msglen, msglen, msgbuf); \
	printf(" reply %d\n", MsgReplyStatus(tid, msgcode));*/ \
}
static void advsrr_child2() {
#if 0
	int tid, msgcode, msglen;
	char msgbuf[32];
	TESTRECV(1, (char *)NULL, 16);
	TESTRECV(2, msgbuf, 16);
	TESTRECV(3, (char *)NULL, 16);
	TESTRECV(4, msgbuf, 32);
	TESTRECV(5, msgbuf, 16);
	TESTRECV(6, msgbuf, 8);
#endif
}
static void advsrr_child1() {
#if 0
	int child2 = Create(1, advsrr_child2);
#endif
	int tid, msgcode, msglen;
	char msgbuf[32];
	TESTRECV(1, (char *)NULL, 16);
	TESTRECV(2, msgbuf, 16);
	TESTRECV(3, (char *)NULL, 16);
	TESTRECV(4, msgbuf, 32);
	TESTRECV(5, msgbuf, 16);
	TESTRECV(6, msgbuf, 8);
#if 0
#define TESTFWD(i,buf,bufsz) { \
	msglen = MsgReceive(&tid, &msgcode, buf, bufsz); \
	printf("Forward #%d (child %d): %d %d\n", i, msgcode, msglen, MsgForward(tid, child2, 2)); \
}
	TESTFWD(1, NULL, 16);
	TESTFWD(2, msgbuf, 16);
	TESTFWD(3, NULL, 16);
	TESTFWD(4, msgbuf, 32);
	TESTFWD(5, msgbuf, 16);
	TESTFWD(6, msgbuf, 8);
#undef TESTFWD
#endif
}
#undef TESTRECV
__attribute__((unused)) static void advsrr_task() {
	int tid = MyTid();
	advsrr_chan1 = ChannelOpen();
	char data[16];
	memcpy(data, "abcdefghijklmnopqrstuvwxyz", 16);
#if 0
	int child1 = Create(0, advsrr_child1);
#endif
#define TESTSEND(i,buf,bufsz) { \
	printf("Send #%d: %d\n", i, MsgSend(advsrr_chan1, 1, buf, bufsz, NULL, 0, NULL)); \
}

	printf("Receive test:\n");
	printf("advsrr_task[%d]: send NULL, recv NULL\n", tid);
	TESTSEND(1, NULL, 64);
	printf("advsrr_task[%d]: send NULL, recv 16\n", tid);
	TESTSEND(2, NULL, 64);
	printf("advsrr_task[%d]: send 16, recv NULL\n", tid);
	TESTSEND(3, data, 16);
	printf("advsrr_task[%d]: send 16, recv 32\n", tid);
	TESTSEND(4, data, 16);
	printf("advsrr_task[%d]: send 16, recv 16\n", tid);
	TESTSEND(5, data, 16);
	printf("advsrr_task[%d]: send 16, recv 8\n", tid);
	TESTSEND(6, data, 16);

#if 0
	printf("\nForward test:\n");
	printf("advsrr_task[%d]: send NULL, recv NULL\n", tid);
	TESTSEND(1, NULL, 64);
	printf("advsrr_task[%d]: send NULL, recv 16\n", tid);
	TESTSEND(2, NULL, 64);
	printf("advsrr_task[%d]: send 16, recv NULL\n", tid);
	TESTSEND(3, data, 16);
	printf("advsrr_task[%d]: send 16, recv 32\n", tid);
	TESTSEND(4, data, 16);
	printf("advsrr_task[%d]: send 16, recv 16\n", tid);
	TESTSEND(5, data, 16);
	printf("advsrr_task[%d]: send 16, recv 8\n", tid);
	TESTSEND(6, data, 16);
#endif
}
#endif

static uint32_t dumbhash(int x) { return 0; }
static void hashtable_print(hashtable *ht) {
	printf("{");
	for(int i=0; i<ht->max; i++) {
		if(!ht->arr[i].valid)
			printf("*");
		else if(ht->arr[i].deleted)
			printf("x");
		else
			printf("%d:%p", ht->arr[i].key, ht->arr[i].value);
		if(i != ht->max-1)
			printf(",");
	}
	printf("}\n");
}
__attribute__((unused)) static void hashtable_test() {
	hashtable ht;
	struct ht_item ht_arr[3];
	hashtable_init(&ht, ht_arr, 3, dumbhash, NULL);
#define GET(i) { int k = (i); printf("get %d -> %d: ", k, hashtable_get(&ht, k, NULL)); hashtable_print(&ht); }
#define PUT(i,j) { int k = (i); void *l = (void *)(j); printf("put %d -> %d: ", k, hashtable_put(&ht, k, l)); hashtable_print(&ht); }
#define DEL(i) { int k = (i); printf("del %d -> %d: ", k, hashtable_del(&ht, k)); hashtable_print(&ht); }
	GET(0); PUT(0,0); GET(0); DEL(0);
	PUT(3,1); PUT(5,2); PUT(0,3); PUT(1,4); PUT(3,0); PUT(5,5); DEL(0); DEL(3); DEL(1); DEL(5);
#undef GET
#undef PUT
#undef DEL
}

/* The first user program */
void userprog_init() {
	ChannelOpen(); /* stdin */
	ChannelOpen(); /* stdout */
	ChannelOpen(); /* fs */

	CreateDaemon(1, consoletx_task);
	CreateDaemon(1, consolerx_task);
	CreateDaemon(1, clockserver_task);
	CreateDaemon(1, ethrx_task);
	CreateDaemon(1, icmpserver_task);
	CreateDaemon(1, arpserver_task);
	CreateDaemon(1, udprx_task);
	CreateDaemon(2, udpconrx_task);
	CreateDaemon(2, fileserver_task);

	printf("hello, world\n");

	dump_files();

	//ASSERTNOERR(Create(0, hashtable_test));
	ASSERTNOERR(Create(1, memcpy_bench));
	//ASSERTNOERR(Create(2, advsrr_task));
	//ASSERTNOERR(Create(2, task_reclamation_test));
	//ASSERTNOERR(Create(2, srr_task));
	//ASSERTNOERR(Create(3, srrbench_task));

	ASSERTNOERR(CreateDaemon(4, flash_leds));
	//ASSERTNOERR(Create(4, console_loop));
	ASSERTNOERR(Create(4, udp_console_loop));
}
