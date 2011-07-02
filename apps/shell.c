#include <string.h>
#include <lib.h>
#include <syscall.h>
#include <apps.h>

typedef int (*cmd_func_t)(int argc, char **argv);

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
	uint64_t start, elapsed;

	ASSERTNOERR(udp_bind(NETSRR_PORT));
	printf("waiting for connection...\n");
	ASSERTNOERR(udp_wait(NETSRR_PORT, &pkt.rec, sizeof(pkt)));
	in_addr_t client_ip = pkt.rec.src_ip;
	uint16_t client_port = pkt.rec.src_port;
	printf("got connection from %08x:%d\n", client_ip, client_port);

	for(int step=0; step<NETSRR_STEPS; ++step) {
		printf("  %d bytes, ", 1<<step);
		start = read_timer();
		for(int i=0; i<NETSRR_RUNS; ++i) {
			ASSERTNOERR(udp_wait(NETSRR_PORT, &pkt.rec, sizeof(pkt)));
			ASSERTNOERR(send_udp(NETSRR_PORT, client_ip, client_port, pkt.payload, pkt.rec.data_len));
		}
		elapsed = read_timer() - start;
		printf("%lld ns\n", elapsed*1000000/TICKS_PER_MSEC/NETSRR_RUNS);
	}

	printf("done!\n");
	ASSERTNOERR(udp_release(NETSRR_PORT));
	return 0;
}
static int netsrr_client_cmd(int argc, char **argv) {
	struct pkt {
		struct packet_rec rec;
		char payload[2048];
	} pkt;
	uint64_t start, elapsed;

	if(argc != 2) {
		printf("Usage: %s client\n", argv[0]);
		return 1;
	}
	const struct hostdata *dest = get_host_data_from_name(argv[1]);
	if(!dest) {
		printf("unknown host %s\n", argv[1]);
		return -1;
	}

	ASSERTNOERR(udp_bind(NETSRR_PORT));
	printf("starting srr to %s (IP: %08x)\n", argv[1], dest->ip);
	ASSERTNOERR(send_udp(NETSRR_PORT, dest->ip, NETSRR_PORT, NULL, 0));
	for(int step=0; step<NETSRR_STEPS; ++step) {
		printf("%d bytes, ", 1<<step);
		start = read_timer();
		for(int i=0; i<NETSRR_RUNS; ++i) {
			ASSERTNOERR(send_udp(NETSRR_PORT, dest->ip, NETSRR_PORT, pkt.payload, 1<<step));
			ASSERTNOERR(udp_wait(NETSRR_PORT, &pkt.rec, sizeof(pkt)));
		}
		elapsed = read_timer() - start;
		printf("%lld ns\n", elapsed*1000000/TICKS_PER_MSEC/NETSRR_RUNS);
	}

	printf("done!\n");
	ASSERTNOERR(udp_release(NETSRR_PORT));
	return 0;
}

static int help_cmd(int argc, char **argv);

#define CMD(name,desc) { #name, name##_cmd, desc }
static struct cmd_def {
	const char *cmd;
	cmd_func_t function;
	const char *desc;
} cmd_defs[] = {
	{"help", help_cmd, "List all commands"},
	{"exit", exit_cmd, "Exit back to U-Boot"},
	{"reset", reset_cmd, "Reset board"},
	{"genesis", genesis_cmd, "Start a task remotely"},
	{"leds", leds_cmd, "Flash the blinkenlights"},
	{"ls", ls_cmd, "List all files in the filesystem"},
	{"ipcbench", ipcbench_cmd, "Benchmark local SRR"},
	{"netsrr_server", netsrr_server_cmd, "Start a server for benchmarking network SRR"},
	{"netsrr_client", netsrr_client_cmd, "Connect to a server to benchmark network SRR"},
};

static int help_cmd(int argc, char **argv) {
	static size_t maxlen = 0;
	if(maxlen == 0) {
		for(size_t i=0; i<arraysize(cmd_defs); ++i) {
			size_t cmdlen = strlen(cmd_defs[i].cmd);
			if(cmdlen > maxlen)
				maxlen = cmdlen;
		}
	}
	for(size_t i=0; i<arraysize(cmd_defs); ++i) {
		printf("%-*s - %s\n", maxlen, cmd_defs[i].cmd, cmd_defs[i].desc);
	}
	return 0;
}

static cmd_func_t command_lookup(char *command) {
	for (size_t i = 0; i < arraysize(cmd_defs); ++i) {
		if(!strcmp(cmd_defs[i].cmd, command))
			return cmd_defs[i].function;
	}
	return NULL;
}

#define IDXP1(x,arr) (((x)+1)%arraysize(arr))
#define IDXM1(x,arr) (((x)+(arraysize(arr))-1)%arraysize(arr))

static void shell_task(void) {
	char history[32][128];    /* canonical history buffer */
	char historyalt[32][128]; /* altered history buffer */
	int histstart=0, histpos=0, histend=0;

	char line[128];
	size_t linepos, endpos, linelen;

	int promptlen;

	while(1) {
		promptlen = printf(
#ifdef BUILDUSER
	STRINGIFY2(BUILDUSER) "@"
#endif
				"%s:/# ", this_host->hostname);
		history[histend][0] = 0;
		historyalt[histend][0] = 0;
		histpos = histend;
linereset:
		linepos = linelen = strlen(historyalt[histpos]);
		endpos = sizeof(line);
		memcpy(line, historyalt[histpos], linelen);
		printf("\033[%dG", promptlen+1); // set cursor to end of prompt
		printf("%s", historyalt[histpos]); // print line
		printf("\033[K"); // kill rest of line
		bool done = false;
		while(!done) {
			char c = getchar();
			switch(c) {
				case '\b':
				case 127:
					if(linepos > 0) {
						printf("\b\033[P"); // delete one character
						--linepos;
						--linelen;
					}
					continue;
				case '\r':
				case '\n':
					putchar('\n');
					done = true;
					break;
				case 3:
					printf("^C\n");
					linepos = linelen = 0;
					endpos = sizeof(line);
					done = true;
					break;
				case 4: // ^D
					if(linelen == 0) {
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
							if(histpos != histstart) {
								sprintf(historyalt[histpos], "%.*s%.*s", linepos, line, linelen-linepos, line+endpos);
								histpos = IDXM1(histpos, history);
								goto linereset;
							} else {
								printf("\a");
							}
							break;
						} else if(c == 'B') {
							/* down arrow */
							if(histpos != histend) {
								sprintf(historyalt[histpos], "%.*s%.*s", linepos, line, linelen-linepos, line+endpos);
								histpos = IDXP1(histpos, history);
								goto linereset;
							} else {
								printf("\a");
							}
							break;
						} else if(c == 'C') {
							/* right arrow */
							if(linepos < linelen) {
								printf("\033[C");
								line[linepos++] = line[endpos++];
							}
							break;
						} else if(c == 'D') {
							/* left arrow */
							if(linepos > 0) {
								printf("\033[D");
								line[--endpos] = line[--linepos];
							}
							break;
						} else {
							/* fall through */
						}
					} else {
						printf("\a");
						/* fall through */
					}
				default:
					if(c == '\t') {
						c = ' ';
					} else if(c < 32) {
						printf("^%c\b\b", c+0x40);
						break;
					}
					if(linelen >= arraysize(history[0])-3) {
						if(linepos < linelen) {
							line[endpos] = c;
						}
						putchar(c);
						putchar('\b');
						break;
					}
					if(linepos == linelen) {
						putchar(c);
					} else {
						printf("\033[@"); // insert blank character
						putchar(c);
					}
					line[linepos++] = c;
					linelen++;
					break;
			}
		}
		if(linelen == 0) {
			/* restore history */
			strcpy(historyalt[histpos], history[histpos]);
			continue;
		}

		char buf[sizeof(history[0])];
		sprintf(buf, "%.*s%.*s", linepos, line, linelen-linepos, line+endpos);

		if(histstart == histend ||
		  strcmp(buf, history[IDXM1(histend, history)]) != 0) {
			/* add a new history entry only if previous entry is not equal */
			memcpy(history[histend], buf, linelen+1);
			memcpy(historyalt[histend], buf, linelen+1);
			histend = IDXP1(histend, history);
			if(histstart == histend)
				histstart = IDXP1(histstart, history);
		}
		/* restore history */
		strcpy(historyalt[histpos], history[histpos]);

		buf[linelen] = '\n';
		buf[linelen+1] = '\0';
		char *argv[10];
		int argc = parse_args(buf, argv, arraysize(argv));
		if(argc == 0)
			continue;

		cmd_func_t command = command_lookup(argv[0]);

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
