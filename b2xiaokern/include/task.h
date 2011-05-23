#ifndef TASK_H
#define TASK_H

#define MAX_TASKS 4096

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

#endif /* TASK_H */
