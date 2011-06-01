/* Put your "user" programs here.
   For ease of modification, put the header files that you need
   right above your function, rather than all at the top; this allows
   functions to be easily added and removed without leaving a ton
   of #includes up top. */
#include <syscall.h>
#include <lib.h>

#include <eth.h>
#include <servers/net.h>
static void udp_console_loop() {
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
static void flash_leds() {
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
static void console_loop() {
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
static void memcpy_bench() {
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

static void srr_child() {
	char buf[4];
	int i, j;
	for(i=0; i<5; i++) {
		int tid;
		int len = Receive(&tid, NULL, buf, sizeof(buf));
		printf("  Child Received, with retval %d\n", len);
		if(len < 0) {
			printf("RECEIVE FAILED: %d\n", len);
			Reply(tid, 6, "FAILED", 6);
			continue;
		}
		printf("  Child got Receive: ");
		for(j=0; j<len; j++)
			putchar(buf[j]);
		printf("\n");
		int ret = Reply(tid, i, "0123456789", i);
		if(ret < 0) {
			printf("  Child Reply FAILED: %d\n", ret);
		} else {
			printf("  Child Reply!\n");
		}
	}
}

static void srr_task() {
	int tid = MyTid();
	printf("srr_task[%d]: testing SRR transaction\n", tid);
	int child = Create(1, srr_child);
	char buf[4];
	int i, j;
	for(i=0; i<5; i++) {
		int len = Send(child, 0, "abcdefghijklmno", i, buf, sizeof(buf));
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
}

#define SRR_RUNS 16384
static void srrbench_child() {
	char buf[1024];
	int tid;
	int i;
	for(i=0; i<SRR_RUNS; i++) {
		Receive(&tid, NULL, buf, 4);
		Reply(tid, 4, buf, 4);
	}
	for(i=0; i<SRR_RUNS; i++) {
		Receive(&tid, NULL, buf, 64);
		Reply(tid, 64, buf, 64);
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

static void srrbench_task() {
	int tid = MyTid();
	printf("srrbench_task[%d]: benchmarking SRR transaction\n", tid);
	int child = Create(0, srrbench_child);
	char buf[512];
	int i;
	unsigned long long start, elapsed;

	BENCH("Pass", Pass())
	BENCH("4-bytes", Send(child, 0, buf, 4, buf, 4))
	BENCH("64-bytes", Send(child, 0, buf, 64, buf, 64))

	printf("srrbench_task[%d]: benchmark finished.\n", tid);
}

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
static void hashtable_test() {
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
	//ASSERTNOERR(Create(0, hashtable_test));
	ASSERTNOERR(Create(1, memcpy_bench));
	//ASSERTNOERR(Create(2, srr_task));
	//ASSERTNOERR(Create(3, srrbench_task));

	ASSERTNOERR(CreateDaemon(4, flash_leds));
	//ASSERTNOERR(Create(4, console_loop));
	ASSERTNOERR(Create(4, udp_console_loop));
}
