#include <string.h>
#include <lib.h>
#include <syscall.h>

#include <apps.h>
#include <servers/genesis.h>
#include <servers/net.h>
#include <servers/fs.h>

static int exit_cmd(int argc, char **argv) {
	printf("Goodbye\n");
	exit();
	return 666; // Unreached
}

static int ls_cmd(int argc, char **argv) {
	dump_files();
	return 0;
}

#include <drivers/leds.h>
#include <servers/clock.h>
static int leds_cmd(int argc, char **argv) {
	printf("Now flashing the blinkenlights.\n");
	for(;;) {
		for(enum leds led = LED1; led <= LED5; led++) {
			led_set(led, 1);
			msleep(100);
			led_set(led, 0);
		}
	}
	return 0;
}

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

#define C(x) { #x, x##_cmd }
static int (*command_lookup(char *command))(int, char**) {
	struct cmd_defs { char cmd[12]; int (*function)(int, char**); }
	cmd_defs[] = {
	C(exit), C(genesis), C(leds), C(ls), C(srrbench)
	};
#undef C
	for (size_t i = 0; i < arraysize(cmd_defs); ++i) {
		if(!strcmp(cmd_defs[i].cmd, command))
			return cmd_defs[i].function;
	}
	return 0;
}

void shell_task(void) {
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
