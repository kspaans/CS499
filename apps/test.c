#include <types.h>
#include <lib.h>
#include <string.h>
#include <syscall.h>
#include <servers/fs.h>

static void null_task(void) {
}
static void task_reclamation_2(void) {
	for(int i=0; i<1000; ++i) {
		printf("%05x ", spawn(5, null_task, 0));
		printf("%05x\n", spawn(5, null_task, 0));
	}
}
__attribute__((unused)) static void task_reclamation_test(void) {
	for(int i=0; i<1000; ++i) {
		printf("%05x ", spawn(1, null_task, 0));
		printf("%05x ", spawn(1, null_task, 0));
		printf("%05x\n", spawn(3, null_task, 0));
	}
	spawn(4, task_reclamation_2, 0);
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
