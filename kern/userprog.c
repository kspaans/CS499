/* Put your "user" programs here.
   For ease of modification, put the header files that you need
   right above your function, rather than all at the top; this allows
   functions to be easily added and removed without leaving a ton
   of #includes up top. */
#include <syscall.h>
#include <lib.h>
#include <kern/printk.h>
#include <servers/fs.h>
#include <applications.h>

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
__attribute__((unused)) static void udp_tx_loop() {
	printf("Type characters to send to the remote host; Ctrl+D to quit\n");

	udp_printf("Hello from %s\n", this_host->hostname);

	for(;;) {
		char c = getchar();
		if(c == 4)
			return;
		if(c == '\r')
			c = '\n';
		udp_printf("%c", c);
	}
}

#include <eth.h>
#include <servers/net.h>
__attribute__((unused)) static void udp_rx_loop() {
	for(;;) {
		char c = udp_getchar();
		printf("%c", c);
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
		yield();
	}
}

#include <lib.h>
#include <string.h>
#include <timer.h>
#include <kern/kmalloc.h>
__attribute__((unused)) static void memcpy_bench() {
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
	int tid = gettid();
	printf("srrbench_task[%d]: benchmarking SRR transaction\n", tid);
	//int child = create(0, srrbench_child, 0);
	char buf[512];
	int i;
	unsigned long long start, elapsed;

	srrbench_chan = channel(0);

	BENCH("yield", yield())
	BENCH("4-bytes", MsgSend(srrbench_chan, 0, buf, 4, buf, 4, NULL))
	BENCH("64-bytes", MsgSend(srrbench_chan, 0, buf, 64, buf, 64, NULL))

	printf("srrbench_task[%d]: benchmark finished.\n", tid);
}

static void nulltask() {
}
static void task_reclamation_2() {
	for(int i=0; i<1000; ++i) {
		printk("%05x ", create(5, nulltask, 0));
		printk("%05x\n", create(5, nulltask, 0));
	}
}
__attribute__((unused)) static void task_reclamation_test() {
	for(int i=0; i<1000; ++i) {
		printk("%05x ", create(1, nulltask, 0));
		printk("%05x ", create(1, nulltask, 0));
		printk("%05x\n", create(3, nulltask, 0));
	}
	create(4, task_reclamation_2, 0);
}

static uint32_t dumbhash(int x) { return 0; }
static void hashtable_print(hashtable *ht) {
	printf("{");
	for(uint32_t i=0; i<ht->max; i++) {
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

__attribute__((unused)) static void fstest_task() {
	int chan;
	char path[PATH_MAX+1];
	printf("path_max test\n");
	memset(path, 'a', PATH_MAX);
	path[PATH_MAX] = 0;
	printf(" mkchan: %d\n", mkchan(ROOT_DIRFD, path));
	printf(" open: %d\n", chan=open(ROOT_DIRFD, path));
	printf(" rmchan: %d\n", rmchan(ROOT_DIRFD, path));
	printf(" close: %d\n", close(chan));
	for(int i=0; i<100; ++i) {
		sprintf(path, "/tmp/test%d", i);
		ASSERTNOERR(mkchan(ROOT_DIRFD, path));
		ASSERTNOERR(chan=open(ROOT_DIRFD, path));
		ASSERTNOERR(rmchan(ROOT_DIRFD, path));
		ASSERTNOERR(close(chan));
	}
}

/* The first user program */
void userprog_init() {
	channel(0); /* stdin */
	channel(0); /* stdout */
	channel(0); /* fs */

	printk("console init\n");

	create(1, consoletx_task, CREATE_DAEMON);
	create(1, consolerx_task, CREATE_DAEMON);

	net_init();

	create(1, clockserver_task, CREATE_DAEMON);
	create(1, ethrx_task, CREATE_DAEMON);
	create(1, icmpserver_task, CREATE_DAEMON);
	create(1, arpserver_task, CREATE_DAEMON);
	create(1, udprx_task, CREATE_DAEMON);
	create(2, udpconrx_task, CREATE_DAEMON);
	create(2, fileserver_task, CREATE_DAEMON);

	dump_files();

	//ASSERTNOERR(create(0, hashtable_test, 0));
	ASSERTNOERR(create(1, memcpy_bench, 0));
	ASSERTNOERR(create(2, fstest_task, 0));
	//ASSERTNOERR(create(2, task_reclamation_test, 0));
	//ASSERTNOERR(create(2, srr_task, 0));
	//ASSERTNOERR(create(3, srrbench_task, 0));

	ASSERTNOERR(create(4, flash_leds, CREATE_DAEMON));
	//ASSERTNOERR(create(4, console_loop, 0));
	ASSERTNOERR(create(4, udp_tx_loop, 0));
	ASSERTNOERR(create(4, udp_rx_loop, CREATE_DAEMON));
//	ASSERTNOERR(create(6, gameoflife, 0));
}
