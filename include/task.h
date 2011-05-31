#ifndef TASK_H
#define TASK_H

#define MAX_TASKS 4096

#define TASK_DAEMON 1
#define TASK_NORMAL 0
#define TID_AUTO -1

/* Publically query-able task state. */

enum taskstate {
	TASK_RUNNING,		/* Active or ready to run */
	TASK_RECV_BLOCKED,	/* Called Send(), waiting for Receive() */
	TASK_REPLY_BLOCKED,	/* Called Send(), waiting for Reply() */
	TASK_SEND_BLOCKED,	/* Called Receive(), waiting for Send() */
	TASK_DEAD,			/* Called Exit() */
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

int reserve_tid();
int KernCreateTask(int priority, void (*code)(), int daemon, int tid);

#endif /* TASK_H */
