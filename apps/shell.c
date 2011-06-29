#include <string.h>
#include <lib.h>
#include <syscall.h>

#include <apps.h>
#include <servers/genesis.h>
#include <servers/net.h>
#include <servers/fs.h>

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

		//void (*command)(int, char**) = command_lookup(argv[0]);
		//command(argc, argv);

		if(!strcmp(argv[0], "exit")) {
			printf("Goodbye\n");
			exit();
		} else if(!strcmp(argv[0], "logout")) {
			exit();
		} else if(!strcmp(argv[0], "ls")) {
			dump_files();
		} else if(!strcmp(argv[0], "genesis")) {
			if(argc != 3) {
				printf("Usage: genesis hostname command\n");
				continue;
			}
			const struct hostdata *dest = get_host_data_from_name(argv[1]);
			if(dest) {
				send_createreq(dest->ip, 2, argv[2], 0);
			} else {
				printf("unknown host\n");
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
