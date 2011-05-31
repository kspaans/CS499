#ifndef KERN_TASK_H
#define KERN_TASK_H

#include <task.h>

typedef void* useraddr_t;
typedef const void* const_useraddr_t;

/* Number of non-daemon tasks. The system exits when no non-daemon tasks remain. */
extern int nondaemon_count;

/* Number of priorities supported.
 * Priority numbers run from 0 (highest priority)
 * to TASK_NPRIO-1 (lowest priority). */
#define TASK_NPRIO          8

/* Size of the stack allocated to each task */
#define TASK_STACKSIZE      65536 /* 64 KB */

#define STACK_ARG(task, n)  (*(((int *)task->regs.sp) + (n-4)))

/* NOTE: Changes to the regs stucture MUST be reflected in switch.S. */
struct regs {
	/* The strange layout of this struct is due to the construction of switch.S.
	   See that file for the gory details. */
	int r0, r1, pc;
	int r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr;
	int psr;
};

/* Forward declaration */
struct task;
typedef struct {
	struct task* start;
	struct task* end;
} taskqueue;

union tasksrr {
	struct {
		int tid;
		int code;
		const_useraddr_t buf;
		int len;
		useraddr_t rbuf;
		int rlen;
	} send;
	struct {
		useraddr_t tidptr;
		useraddr_t codeptr;
		useraddr_t buf;
		int len;
	} recv;
	struct {
		int id;
	} event;
};

struct task {
	/* regs must be first */
	struct regs regs;
	int tid;
	struct task *parent;
	int priority;
	int daemon;
	int stack_start;
	enum taskstate state;
	/* Receive queue: holds tasks which have sent to this one
	 * but which haven't been received by this task yet */
	taskqueue recv_queue;
	union tasksrr srr;

	/* for putting tasks in queues */
	struct task *prevtask;
	struct task *nexttask;
};

/* Initialize task system */
void init_tasks();
/* Get the initial PSR for user functions */
int get_user_psr(); /* asm function */
/* Activate the specified task, allowing it to run */
void task_activate(struct task *task); /* asm function */
/* SWI and IRQ handlers */
void task_syscall(int code, struct task *task); /* in task.c */
void task_irq(); /* in interrupt.c */
/* Get the next ready task */
struct task *task_dequeue();
/* Enqueue a ready task */
void task_enqueue(struct task *task);
/* Lookup a task given its TID */
struct task *get_task(int tid);
/* Check for stack overflow */
void check_stack(struct task* task);

int get_num_tasks();

/* Task queue management */
void taskqueue_init(taskqueue* queue);
struct task* taskqueue_pop();
void taskqueue_push(taskqueue* queue, struct task* task);
int taskqueue_empty(taskqueue* queue);

/// debug
void print_task(struct task *task);
/// debug

/* System calls */
int syscall_Create(struct task *task, int priority, void (*code)());
int syscall_CreateDaemon(struct task *task, int priority, void (*code)());
int syscall_MyTid(struct task *task);
int syscall_MyParentTid(struct task *task);
void syscall_Pass(struct task *task);
void syscall_Exit(struct task *task);
int syscall_Send(struct task *task, int tid, int msgcode, const_useraddr_t msg, int msglen, useraddr_t reply, int replylen);
int syscall_Receive(struct task *task, useraddr_t tid, useraddr_t msgcode, useraddr_t msg, int msglen);
int syscall_Reply(struct task *task, int tid, int status, const_useraddr_t reply, int replylen);
int syscall_AwaitEvent(struct task* task, int eventid);
int syscall_TaskStat(struct task* task, int tid, useraddr_t stat);
/* Kernel-internal calls */
int do_Create(struct task *task, int priority, void (*code)(), int daemon, int tid);

void event_unblock_all(int eventid, int return_value);
void event_unblock_one(int eventid, int return_value);

#endif /* KERN_TASK_H */
