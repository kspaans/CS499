#ifndef TASK_H
#define TASK_H

#define MAX_TASKS 4095 /* excluding "kernel task" 0 */

#define TASK_DAEMON 1
#define TASK_NORMAL 0

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define ROOT_DIRFD 2

/* Publically query-able task state. */

enum taskstate {
	TASK_RUNNING,		/* Active or ready to run */
	TASK_SEND_BLOCKED,	/* blocked in sys_send */
	TASK_RECV_BLOCKED,	/* blocked in sys_recv */
	TASK_DEAD,			/* Called sys_exit() */
	TASK_EVENT_BLOCKED,
};

struct task_stat {
	int tid;
	int ptid;
	int priority;
	int daemon;
	enum taskstate state;
	int srrtid;
};

#endif /* TASK_H */
