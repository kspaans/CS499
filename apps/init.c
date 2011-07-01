#include <syscall.h>
#include <lib.h>
#include <kern/printk.h>
#include <servers/clock.h>
#include <servers/console.h>
#include <servers/fs.h>
#include <servers/genesis.h>
#include <servers/net.h>
#include <apps.h>

void init_task(void) {
	channel(0); /* stdin */
	channel(0); /* stdout */
	channel(0); /* fs */

	console_init();
	fileserver_init();
	clockserver_init();
	net_init();

	// any servers above this point will not be be able to use the netconsole
	if (!this_host->has_uart) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		open(ROOT_DIRFD, "/dev/netconin");
		open(ROOT_DIRFD, "/dev/netconout");
	}

	genesis_init();
	shell_init();
}
