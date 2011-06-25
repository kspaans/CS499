/* Put your "user" programs here.
   For ease of modification, put the header files that you need
   right above your function, rather than all at the top; this allows
   functions to be easily added and removed without leaving a ton
   of #includes up top. */
#include <syscall.h>
#include <lib.h>
#include <kern/printk.h>
#include <servers/fs.h>
#include <servers/console.h>
#include <servers/net.h>
#include <apps.h>
#include <id.h>

#include <eth.h>
#include <servers/net.h>
__attribute__((unused)) static void udp_tx_loop(void) {
	printf("Type characters to send to the remote host; Ctrl+D to quit\n");

	int netconout = open(ROOT_DIRFD, "/dev/netconout");

	fprintf(netconout, "Hello from %s\n", this_host->hostname);

	for(;;) {
		char c = getchar();
		if(c == 4)
			return;
		if(c == '\r')
			c = '\n';
		fputc(c, STDOUT_FILENO);
		fputc(c, netconout);
	}
}

#include <eth.h>
#include <servers/net.h>
__attribute__((unused)) static void udp_rx_loop(void) {
	int netconin = open(ROOT_DIRFD, "/dev/netconin");
	int netconout = open(ROOT_DIRFD, "/dev/netconout");

	for(;;) {
		char c = fgetc(netconin);
		fputc(c, STDOUT_FILENO);
		fputc(c, netconout);
	}
}

#include <drivers/leds.h>
#include <servers/clock.h>
__attribute__((unused)) static void flash_leds(void) {
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
__attribute__((unused)) static void console_loop(void) {
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
#define BENCHSRR(n) { iov[0].iov_len = n; for(i=0; i<SRR_RUNS; ++i) { sys_recv(3, iov, arraysize(iov), NULL, 0); sys_send(3, iov, arraysize(iov), -1, 0); } }
	BENCHSRR(1)
	BENCHSRR(2)
	BENCHSRR(4)
	BENCHSRR(8)
	BENCHSRR(16)
	BENCHSRR(32)
	BENCHSRR(64)
	BENCHSRR(128)
	BENCHSRR(256)
	BENCHSRR(512)
#undef BENCHSRR
}
#include <drivers/timers.h>
#define BENCH(name, init, code) { \
	printf("SRR Benchmarking: " name ": "); \
	start = read_timer(); \
	init; \
	for(i=0; i<SRR_RUNS; i++) code; \
	elapsed = read_timer()-start; \
	printf("%d ms ", (int)(elapsed/TICKS_PER_MSEC)); \
	printf("(%d ns/loop)\n", (int)(elapsed*1000000/TICKS_PER_MSEC/SRR_RUNS)); \
}
__attribute__((unused)) static void srrbench_task(void) {
	int tid = gettid();
	printk("srrbench_task[%d]: benchmarking SRR transaction\n", tid);

	int srrbench_chan = channel(0);

	int chans[] = { 0, 1, 2, srrbench_chan };
	printk("%d\n", sys_spawn(0, srrbench_child, chans, arraysize(chans), 0));

	char buf[512];
	int i;
	unsigned long long start, elapsed;

	struct iovec iov[] = { { buf, sizeof(buf) } };

	BENCH("yield", (void)0,yield())
#define BENCHSRR(n) BENCH(#n "-bytes", iov[0].iov_len = n, ({sys_send(srrbench_chan, iov, arraysize(iov), -1, 0); sys_recv(srrbench_chan, iov, arraysize(iov), NULL, 0); }))
	BENCHSRR(1)
	BENCHSRR(2)
	BENCHSRR(4)
	BENCHSRR(8)
	BENCHSRR(16)
	BENCHSRR(32)
	BENCHSRR(64)
	BENCHSRR(128)
	BENCHSRR(256)
	BENCHSRR(512)
#undef BENCHSRR

	printf("srrbench_task[%d]: benchmark finished.\n", tid);
}

static void nulltask(void) {
}
static void task_reclamation_2(void) {
	for(int i=0; i<1000; ++i) {
		printk("%05x ", spawn(5, nulltask, 0));
		printk("%05x\n", spawn(5, nulltask, 0));
	}
}
__attribute__((unused)) static void task_reclamation_test(void) {
	for(int i=0; i<1000; ++i) {
		printk("%05x ", spawn(1, nulltask, 0));
		printk("%05x ", spawn(1, nulltask, 0));
		printk("%05x\n", spawn(3, nulltask, 0));
	}
	spawn(4, task_reclamation_2, 0);
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
__attribute__((unused)) static void hashtable_test(void) {
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

__attribute__((unused)) static void fstest_task(void) {
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

#include <servers/genesis.h>
#include <string.h>
__attribute__((unused)) static void shell(void) {
	char input[128];
	char *argv[10];
	size_t pos;

	while(1) {
		printf(
#ifdef BUILDUSER
	STRINGIFY2(BUILDUSER) "@"
#endif
				"%s:/# ", this_host->hostname);
		pos = 0;
		bool done = false;
		while(!done) {
			char c = getchar();
			switch(c) {
				case '\b':
				case 127:
					if(pos > 0) {
						printf("\b \b");
						--pos;
					}
					continue;
				case '\r':
				case '\n':
					putchar('\n');
					input[pos++] = '\n';
					input[pos++] = '\0';
					done = true;
					break;
				case 3:
					printf("^C\n");
					pos = 0;
					done = true;
					break;
				case 4: // ^D
					if(pos == 0) {
						printf("logout\n");
						exit();
					}
					break;
				case '\033':
					c = getchar();
					if(c == '[') {
						c = getchar();
						if(c == 'A') {
							/* up arrow */
							break;
						} else if(c == 'B') {
							/* down arrow */
							break;
						} else if(c == 'C') {
							/* right arrow */
							break;
						} else if(c == 'D') {
							/* left arrow */
							break;
						} else {
							/* fall through */
						}
					} else {
						printf("\a");
						/* fall through */
					}
				case '\t':
					c = ' ';
				default:
					putchar(c);
					input[pos++] = c;
					if(pos >= sizeof(input)-3) {
						--pos;
						printf("\b");
					}
					break;
			}
		}
		if(pos == 0)
			continue;
		int argc = parse_args(input, argv, arraysize(argv));
		if(argc == 0)
			continue;
		if(!strcmp(argv[0], "exit")) {
			printf("Goodbye\n");
			exit();
		} else if(!strcmp(argv[0], "logout")) {
			exit();
		} else if(!strcmp(argv[0], "ls")) {
			dump_files();
		} else if(!strcmp(argv[0], "genesis")) {
			send_createreq(IP(10, 0, 0, 10 + *argv[1] - '0'), 2, genesis_test, 0);
			printf("OK\n");
		} else {
			printf("argc=%d", argc);
			for(int i=0; i<argc; ++i) {
				printf(" argv[%d]=%s", i, argv[i]);
			}
			printf("\n");
		}
	}
}

/* The first user program */
void init_task(void) {
	printk("build id ");
	for (int i = 0; i < 20; ++i)
		printk("%02x", build_sha1[i]);
	printk("\n");

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

	ASSERTNOERR(spawn(4, flash_leds, SPAWN_DAEMON));
	//ASSERTNOERR(spawn(4, console_loop, 0));
	ASSERTNOERR(spawn(3, genesis_task, SPAWN_DAEMON));
	ASSERTNOERR(spawn(5, shell, 0));
	//ASSERTNOERR(spawn(6, gameoflife, 0));
}
