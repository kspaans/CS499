#include <string.h>
#include <lib.h>
#include <syscall.h>
#include <apps.h>

#include <panic.h>
static int reset_cmd(int argc, char **argv) {
	printf("Resetting...\n");
	volatile int i = 10000000;
	while(i-->0)
		;
	prm_reset();
}

static int ipcbench_cmd(int arc, char **argv) {
	spawn(0, ipc_bench_task, 0);
	return 0;
}

static int exit_cmd(int argc, char **argv) {
	printf("Goodbye\n");
	exit();
	return 666; // Unreached
}

#include <servers/fs.h>
static int ls_cmd(int argc, char **argv) {
	dump_files();
	return 0;
}

#include <drivers/leds.h>
#include <servers/clock.h>
static void leds_task(void) {
	printf("Now flashing the blinkenlights.\n");
	for(;;) {
		for(enum leds led = LED1; led <= LED5; led++) {
			led_set(led, 1);
			msleep(100);
			led_set(led, 0);
		}
	}
}
static int leds_cmd(int argc, char **argv) {
	int ret = spawn(2, leds_task, SPAWN_DAEMON);
	if(ret < 0)
		return ret;
	return 0;
}

#include <servers/genesis.h>
#include <servers/net.h>
static int genesis_cmd(int argc, char **argv) {
	if(argc != 3) {
		printf("Usage: genesis hostname command\n");
		return 1;
	}
	const struct hostdata *dest = get_host_data_from_name(argv[1]);
	if(dest) {
		send_createreq(dest->ip, 2, argv[2], 0);
		return 0;
	} else {
		printf("unknown host\n");
		return -1;
	}
}

static int srrbench_cmd(int argc, char **argv) {
	return -128;
}

#define NETSRR_PORT 47712
#define NETSRR_STEPS 11
#define NETSRR_RUNS 1024
#include <servers/net.h>
#include <timer.h>
static int netsrr_server_cmd(int argc, char **argv) {
	struct pkt {
		struct packet_rec rec;
		char payload[2048];
	} pkt;
	unsigned long long start, elapsed;

	udp_bind(NETSRR_PORT);
	printf("waiting for connection...\n");
	ASSERTNOERR(udp_wait(NETSRR_PORT, &pkt.rec, sizeof(pkt)));
	in_addr_t client_ip = pkt.rec.src_ip;
	uint16_t client_port = pkt.rec.src_port;
	printf("got connection from %08x:%d\n", client_ip, client_port);

	for(int step=0; step<NETSRR_STEPS; ++step) {
		printf("  %d bytes, ", 1<<step);
		start = read_timer();
		for(int i=0; i<NETSRR_RUNS; ++i) {
			udp_wait(NETSRR_PORT, &pkt.rec, sizeof(pkt));
			send_udp(NETSRR_PORT, client_ip, client_port, pkt.payload, pkt.rec.data_len);
		}
		elapsed = read_timer() - start;
		printf("%lld ns\n", elapsed*1000000/TICKS_PER_MSEC/NETSRR_RUNS);
	}

	printf("done!\n");
	udp_release(NETSRR_PORT);
	return 0;
}
static int netsrr_client_cmd(int argc, char **argv) {
	struct pkt {
		struct packet_rec rec;
		char payload[2048];
	} pkt;
	unsigned long long start, elapsed;

	if(argc != 2) {
		printf("Usage: %s client\n", argv[0]);
		return 1;
	}
	const struct hostdata *dest = get_host_data_from_name(argv[1]);
	if(!dest) {
		printf("unknown host %s\n", argv[1]);
		return -1;
	}

	udp_bind(NETSRR_PORT);
	printf("starting srr to %s (IP: %08x)\n", argv[1], dest->ip);
	ASSERTNOERR(send_udp(NETSRR_PORT, dest->ip, NETSRR_PORT, NULL, 0));
	for(int step=0; step<NETSRR_STEPS; ++step) {
		printf("%d bytes, ", 1<<step);
		start = read_timer();
		for(int i=0; i<NETSRR_RUNS; ++i) {
			send_udp(NETSRR_PORT, dest->ip, NETSRR_PORT, pkt.payload, 1<<step);
			udp_wait(NETSRR_PORT, &pkt.rec, sizeof(pkt));
		}
		elapsed = read_timer() - start;
		printf("%lld ns\n", elapsed*1000000/TICKS_PER_MSEC/NETSRR_RUNS);
	}

	printf("done!\n");
	udp_release(NETSRR_PORT);
	return 0;
}

#define C(x) { #x, x##_cmd }
static int (*command_lookup(char *command))(int, char**) {
	static struct cmd_defs { char cmd[32]; int (*function)(int, char**); }
	cmd_defs[] = {
	C(reset), C(exit), C(genesis), C(leds), C(ls), C(srrbench),
	C(netsrr_server), C(netsrr_client), C(ipcbench)
	};
#undef C
	for (size_t i = 0; i < arraysize(cmd_defs); ++i) {
		if(!strcmp(cmd_defs[i].cmd, command))
			return cmd_defs[i].function;
	}
	return 0;
}

static void shell_task(void) {
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

		int (*command)(int, char**) = command_lookup(argv[0]);

		if(command) {
			int ret = command(argc, argv);
			if(ret != 0) {
				printf("Nonzero return: %d\n", ret);
			}
		} else {
			printf("argc=%d", argc);
			for(int i=0; i<argc; ++i) {
				printf(" argv[%d]=%s", i, argv[i]);
			}
			printf("\n");
		}
	}
}

void shell_init(void) {
	xspawn(5, shell_task, 0);
}
