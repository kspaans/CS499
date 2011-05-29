/* Put your "user" programs here.
   For ease of modification, put the header files that you need
   right above your function, rather than all at the top; this allows
   functions to be easily added and removed without leaving a ton
   of #includes up top. */
#include <kern/printk.h>
#include <syscall.h>
#include <lib.h>

#include <machine.h>
#include <drivers/eth.h>
#include <ip.h>
#define UDPMTU 1400
static mac_addr_t gateway_mac;
static int gateway_mac_init = 0;
static void printfunc_udp(void *data, const char *buf, size_t len) {
	if(!gateway_mac_init) {
		gateway_mac = arp_lookup(IP(10,0,0,1));
		gateway_mac_init = 1;
	}
	for(int i=0; i<len; i+=UDPMTU) {
		int chunk = (i+UDPMTU < len) ? UDPMTU : len-i;
		send_udp(gateway_mac, IP(10,0,0,1), 7022, buf+i, chunk);
	}
}
#undef UDPMTU
#define udp_printf(...) func_printf(printfunc_udp, NULL, __VA_ARGS__)
static void udp_console_loop() {
	printk("Type characters to send to the remote host; Ctrl+D to quit\n");

	mac_addr_t my_mac = eth_mac_addr(ETH1_BASE);
	udp_printf("Hello from ");
	for(int i=0; i<5; i++) {
		udp_printf("%02x:", my_mac.addr[i]);
	}
	udp_printf("%02x\n", my_mac.addr[5]);

	for(;;) {
		char c = getchar();
		if(c == 4)
			return;
		udp_printf("%c", c);
	}
}
#undef udp_printf

#include <drivers/leds.h>
#include <timer.h>
static void flash_leds() {
	printk("Now flashing the blinkenlights.\n");
	for(;;) {
		for(enum leds led = LED1; led <= LED5; led++) {
			led_set(led, 1);
			udelay(100000);
			led_set(led, 0);
		}
		Pass();
	}
}

#include <machine.h>
#include <drivers/uart.h>
static void console_loop() {
	volatile uint32_t *flags, *data;
	flags = (uint32_t *)(UART3_PHYS_BASE + UART_LSR_OFFSET);
	data = (uint32_t *)(UART3_PHYS_BASE + UART_RBR_OFFSET);

	printk("Press q to quit.\n");
	for(;;) {
		if(*flags & UART_DRS_MASK) {
			char c = getchar();
			if(c == 'q')
				return;
		}
		Pass();
	}
}

#include <ip.h>
static void udp_test() {
	mac_addr_t dest = arp_lookup(IP(10,0,0,1));
	printk("received mac: ");
	for(int i=0; i<5; i++) {
		printk("%02x:", dest.addr[i]);
	}
	printk("%02x\n", dest.addr[5]);
	printk("UDP STATUS: %08x\n", send_udp(dest, IP(10,0,0,1), 12345, "abcd", 4));
}

#include <kern/backtrace.h>
#include <lib.h>
#include <string.h>
#include <drivers/timers.h>
static void memcpy_bench() {
	backtrace();
	int tid = MyTid();
	printk("memcpy_bench[%d]: benchmarking memcpy\n", tid);
	/* Run some benchmarks! */
	char buf[1<<14];
	char buf2[1<<14];

	int i;
	unsigned long long start_time = read_timer();
	for(i=0; i<(1<<8); i++) {
		memcpy(buf, buf2, sizeof(buf));
	}
	unsigned long long duration = read_timer() - start_time;
	printk("Did 4MB in %lu milliseconds\n", (unsigned long)(duration/TICKS_PER_MSEC));
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

#include <drivers/timers.h>
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

/* The first user program */
void userprog_init() {
	ASSERTNOERR(Create(1, memcpy_bench));
	ASSERTNOERR(Create(0, udp_test));

	ASSERTNOERR(CreateDaemon(4, flash_leds));
	//ASSERTNOERR(Create(4, console_loop));
	ASSERTNOERR(Create(4, udp_console_loop));
}
