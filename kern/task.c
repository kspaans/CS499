#include <lib.h>
#include <string.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <kern/task.h>
#include <kern/syscallno.h>
#include <kern/kmalloc.h>
#include <kern/printk.h>

static int next_tid;
static taskqueue taskqueues[TASK_NPRIO];

static taskqueue eventqueues[NEVENTS];

static struct task *task_lookup[MAX_TASKS];

/* visible to kernel main */
int nondaemon_count;

void init_tasks() {
	int i;
	/* Initialize task queues */
	for(i=0; i<TASK_NPRIO; i++) {
		taskqueue_init(&taskqueues[i]);
	}
	for(i=0; i<NEVENTS; i++) {
		taskqueue_init(&eventqueues[i]);
	}

	next_tid = 1;
	nondaemon_count = 0;
}

/* Cheap hack way to look up a TID.
 * Change this if TID allocation is changed. */
struct task *get_task(int tid) {
	if(tid <= 0) return NULL;
	if(tid >= next_tid) return NULL;
	return task_lookup[tid-1];
}

int get_num_tasks() {
	return next_tid - 1;
}

struct task *task_dequeue() {
	int i;
	for(i=0; i<TASK_NPRIO; i++) {
		if(taskqueues[i].start != NULL) {
			return taskqueue_pop(&taskqueues[i]);
		}
	}
	return NULL;
}

void task_enqueue(struct task *task) {
	if(task->state != TASK_RUNNING)
		return;
	taskqueue_push(&taskqueues[task->priority], task);
}

void check_stack(struct task* task) {
	if(task->regs.sp<task->stack_start) {
		printk("Error: Task %d has overflowed its stack.\n", task->tid);
	}
}

void taskqueue_init(taskqueue* queue) {
	queue->start = queue->end = NULL;
}

struct task* taskqueue_pop(taskqueue* queue) {
	struct task* ret = queue->start;
	queue->start = ret->nexttask;
	if(queue->start != NULL) {
		queue->start->prevtask = NULL;
	} else {
		queue->end = NULL;
	}
	ret->nexttask = NULL;
	return ret;
}

void taskqueue_push(taskqueue* queue, struct task* task) {
	if(queue->end == NULL) {
		queue->start = queue->end = task;
		task->nexttask = task->prevtask = NULL;
	} else {
		queue->end->nexttask = task;
		task->nexttask = NULL;
		task->prevtask = queue->end;
		queue->end = task;
	}
}

static void __attribute__((noreturn)) task_run(void (*code)()) {
	code();
	Exit();
}

int reserve_tid() {
	return next_tid++;
}

int do_Create(struct task *task, int priority, void (*code)(), int daemon, int tid) {
	if(priority < 0 || priority >= TASK_NPRIO)
		return ERR_CREATE_BADPRIO;

	/* TODO: When free lists are implemented, try the free lists instead of
	 * dying here. */
	struct task *newtask = kmalloc(sizeof(struct task));
	if(newtask == NULL)
		return ERR_CREATE_NOMEM;

	void *newtask_stack = kmalloc(TASK_STACKSIZE);
	if(newtask_stack == NULL)
		return ERR_CREATE_NOMEM;

	if(daemon != TASK_DAEMON)
		nondaemon_count++;

	if(tid == -1)
		tid = reserve_tid();
	task_lookup[tid-1] = newtask;
	newtask->tid = tid;
	newtask->parent = task;
	newtask->priority = priority;
	newtask->daemon = (daemon == TASK_DAEMON);
	newtask->state = TASK_RUNNING;
	newtask->stack_start = (int)newtask_stack;
	taskqueue_init(&newtask->recv_queue);

	/* Set up registers. */
	newtask->regs.pc = (int)task_run;
	newtask->regs.psr = get_user_psr();
	newtask->regs.r0 = (int)code;
	newtask->regs.sp = (int)(newtask_stack) + TASK_STACKSIZE;
	/* set fp and lr to 0 to make backtrace happy */
	newtask->regs.fp = 0;
	newtask->regs.lr = 0;
	task_enqueue(newtask);
	return newtask->tid;
}

int syscall_Create(struct task *task, int priority, void (*code)()) {
	return do_Create(task, priority, code, TASK_NORMAL, -1); // create non-daemon task
}

int syscall_CreateDaemon(struct task *task, int priority, void (*code)()) {
	return do_Create(task, priority, code, TASK_DAEMON, -1); // create daemon task
}

int syscall_MyTid(struct task *task) {
	return task->tid;
}

int syscall_MyParentTid(struct task *task) {
	if(task->parent != NULL)
		return task->parent->tid;
	return 0;
}

void syscall_Pass(struct task *task) {
	/* Do nothing. */
}

void syscall_Exit(struct task *task) {
	task->state = TASK_DEAD;
	if(!task->daemon)
		nondaemon_count--;
	/* Clean out the send queue. */
	struct task *sender;
	while(task->recv_queue.start != NULL) {
		sender = taskqueue_pop(&task->recv_queue);
		if(sender->state != TASK_RECV_BLOCKED) {
			printk("KERNEL FATAL: Exiting task had a non-blocked task on receive queue.\n");
		} else {
			/* Unblock sender and tell it the transaction failed. */
			sender->state = TASK_RUNNING;
			sender->regs.r0 = ERR_SEND_SRRFAIL;
			task_enqueue(sender);
		}
	}
}

/* If we ever implement Destroy, some changes need to be made.
 * - Destroy must remove tasks from the queue that they are on.
 *   - This requires a task to know which queue it is on.
 * - Destroy adds tasks to a free queue.
 *   - Create must allocate from this queue if the available memory is exhausted.
 * - Destroy must clean out the senders (currently already done in Exit)
 * - Destroy may need to kill or reparent all its children
 *   - Requires Destroy to maintain a list of its children
 */

/* Handle a receive transaction.
   Returns 0 if the sender unblocks and 1 if the receiver unblocks. */
static int handle_receive(struct task *receiver, struct task *sender) {
	/* Copy the message */
	if(receiver->srr.recv.buf && sender->srr.send.buf) {
		if(receiver->srr.recv.len < sender->srr.send.len) {
			/* Unblock sender and tell him the send failed */
			sender->regs.r0 = ERR_SEND_NOSPACE;
			sender->state = TASK_RUNNING;
			return 0;
		}
		memcpy(receiver->srr.recv.buf, sender->srr.send.buf, sender->srr.send.len);
	}

	/* Copy the status data */
	if(receiver->srr.recv.tidptr)
		memcpy(receiver->srr.recv.tidptr, (void *)&sender->tid, sizeof(int));
	if(receiver->srr.recv.codeptr)
		memcpy(receiver->srr.recv.codeptr, (void *)&sender->srr.send.code, sizeof(int));

	sender->state = TASK_REPLY_BLOCKED;
	receiver->regs.r0 = sender->srr.send.len;
	receiver->state = TASK_RUNNING;
	return 1;
}

/* Message passing */
int syscall_Send(struct task *sender, int tid, int msgcode, const_useraddr_t msg, int msglen, useraddr_t reply, int replylen) {
	struct task *receiver = get_task(tid);
	if(receiver == NULL)
		return ERR_SEND_BADTID;
	if(receiver->tid != tid || receiver->state == TASK_DEAD)
		return ERR_SEND_NOSUCHTID;

	sender->state = TASK_RECV_BLOCKED;
	sender->srr.send.tid = tid;
	sender->srr.send.code = msgcode;
	sender->srr.send.buf = msg;
	sender->srr.send.len = msglen;
	sender->srr.send.rbuf = reply;
	sender->srr.send.rlen = replylen;
	if(receiver->state == TASK_SEND_BLOCKED) {
		if(handle_receive(receiver, sender))
			task_enqueue(receiver); // receiver unblocked
		else
			return sender->regs.r0; // sender unblocked
	} else {
		taskqueue_push(&receiver->recv_queue, sender);
	}
	/* Return SRRFAIL here. Real return value will be posted by syscall_Reply. */
	return ERR_SEND_SRRFAIL;
}

int syscall_Receive(struct task *receiver, useraddr_t tid, useraddr_t msgcode, useraddr_t msg, int msglen) {
	receiver->state = TASK_SEND_BLOCKED;
	receiver->srr.recv.tidptr = tid;
	receiver->srr.recv.codeptr = msgcode;
	receiver->srr.recv.buf = msg;
	receiver->srr.recv.len = msglen;
	while(receiver->recv_queue.start != NULL) {
		struct task *sender = taskqueue_pop(&receiver->recv_queue);
		if(sender->state != TASK_RECV_BLOCKED) {
			printk("KERNEL FATAL: Task had a non-blocked task on receive queue.\n");
			return ERR_RECEIVE_SRRFAIL;
		}
		if(handle_receive(receiver, sender))
			return receiver->regs.r0; // receiver unblocked
		else
			task_enqueue(sender); // sender unblocked
	}
	return ERR_RECEIVE_SRRFAIL;
}

int syscall_Reply(struct task *task, int tid, int status, const_useraddr_t reply, int replylen) {
	struct task *sender = get_task(tid);
	if(sender == NULL)
		return ERR_REPLY_BADTID;
	if(sender->tid != tid || sender->state == TASK_DEAD)
		return ERR_REPLY_NOSUCHTID;
	if(sender->state != TASK_REPLY_BLOCKED)
		return ERR_REPLY_NOTBLOCKED;

	int ret = 0;
	/* Copy the message. */
	if(sender->srr.send.rbuf && reply) {
		if(sender->srr.send.rlen < replylen) {
			ret = status = ERR_REPLY_NOSPACE;
		} else {
			memcpy(sender->srr.send.rbuf, reply, replylen);
		}
	}
	/* Push sender return value and unblock it */
	sender->regs.r0 = status;
	sender->state = TASK_RUNNING;
	task_enqueue(sender);
	return ret;
}

void event_unblock_all(int eventid, int return_value) {
	while(eventqueues[eventid].start != NULL) {
		event_unblock_one(eventid, return_value);
	}
}

void event_unblock_one(int eventid, int return_value) {
	if(eventqueues[eventid].start != NULL) {
		struct task *task = taskqueue_pop(&eventqueues[eventid]);
		task->regs.r0 = return_value;
		task->state = TASK_RUNNING;
		task_enqueue(task);
	}
}

int syscall_AwaitEvent(struct task *task, int eventid) {
	if(eventid < 0 || eventid >= NEVENTS) {
		return ERR_AWAITEVENT_INVALIDEVENT;
	}
	task->state = TASK_EVENT_BLOCKED;
	task->srr.event.id = eventid;
	taskqueue_push(&eventqueues[eventid], task);

	// Return SRRFAIL... Real value will be posted when unblocked.
	return ERR_AWAITEVENT_SRRFAIL;
}

int syscall_TaskStat(struct task *task, int tid, useraddr_t stat) {
	struct task_stat st;
	if(tid == 0) {
		st.tid = 0;
		st.ptid = next_tid; /* let the caller know how many TIDs exist */
		st.priority = 0;
		st.daemon = 0;
		st.state = TASK_RUNNING;
		st.srrtid = -1;
	} else {
		struct task *othertask = get_task(tid);
		if(othertask == NULL) {
			return ERR_TASKSTAT_BADTID;
		}
		st.tid = othertask->tid;
		if(othertask->parent) st.ptid = othertask->parent->tid;
		else st.ptid = 0;
		st.priority = othertask->priority;
		st.daemon = othertask->daemon;
		st.state = othertask->state;
		if(st.state == TASK_RUNNING || st.state == TASK_DEAD) {
			st.srrtid = -1;
		} else if(st.state == TASK_SEND_BLOCKED) {
			st.srrtid = 0;
		} else if(st.state == TASK_EVENT_BLOCKED) {
			st.srrtid = othertask->srr.event.id;
		} else {
			st.srrtid = othertask->srr.send.tid;
		}
	}
	memcpy(stat, (void *)&st, sizeof(struct task_stat));
	return 0;
}

void task_syscall(int code, struct task *task) {
	int ret;
	switch (code) {
	case SYS_CREATE:
		ret = syscall_Create(task, task->regs.r0, (void *)task->regs.r1);
		break;
	case SYS_CREATEDAEMON:
		ret = syscall_CreateDaemon(task, task->regs.r0, (void *)task->regs.r1);
		break;
	case SYS_MYTID:
		ret = syscall_MyTid(task);
		break;
	case SYS_MYPTID:
		ret = syscall_MyParentTid(task);
		break;
	case SYS_PASS:
		syscall_Pass(task);
		ret = 0;
		break;
	case SYS_EXIT:
		syscall_Exit(task);
		ret = 0;
		break;
	case SYS_SEND:
		ret = syscall_Send(task, /*tid*/task->regs.r0, /*msgcode*/task->regs.r1,
				/*msg*/(useraddr_t)task->regs.r2, /*msglen*/task->regs.r3,
				/*reply*/(useraddr_t)STACK_ARG(task, 4), /*replylen*/STACK_ARG(task, 5));
		break;
	case SYS_RECEIVE:
		ret = syscall_Receive(task, /*tid*/(useraddr_t)task->regs.r0,
				/*msgcode*/(useraddr_t)task->regs.r1,
				/*msg*/(useraddr_t)task->regs.r2, /*msglen*/task->regs.r3);
		break;
	case SYS_REPLY:
		ret = syscall_Reply(task, /*tid*/task->regs.r0, /*status*/task->regs.r1,
				/*reply*/(useraddr_t)task->regs.r2, /*replylen*/task->regs.r3);
		break;
	case SYS_AWAITEVENT:
		ret = syscall_AwaitEvent(task, task->regs.r0);
		break;
	case SYS_TASKSTAT:
		ret = syscall_TaskStat(task, task->regs.r0, (useraddr_t)task->regs.r1);
		break;
	default:
		ret = ERR_BADCALL;
	}
	task->regs.r0 = ret;
}

/// debug
#define PRINT_REG(x) printk(#x ":%08x ", task->regs.x)
void print_task(struct task *task) {
	printk("TASK PRINTOUT: %p\n", task);
	if(task == NULL) return;
	printk("REGS:\n");
	PRINT_REG(r0);
	PRINT_REG(r1);
	PRINT_REG(r2);
	PRINT_REG(r3);
	PRINT_REG(r4);
	PRINT_REG(r5);
	kputs("");
	PRINT_REG(r6);
	PRINT_REG(r7);
	PRINT_REG(r8);
	PRINT_REG(r9);
	PRINT_REG(sl);
	PRINT_REG(fp);
	kputs("");
	PRINT_REG(ip);
	PRINT_REG(sp);
	PRINT_REG(lr);
	PRINT_REG(pc);
	PRINT_REG(psr);
	kputs("");
	printk("STATE:\n");
	printk("tid:%d parent:%p prio:%d, state:%d\n\n", task->tid, task->parent, task->priority, task->state);
}
/// debug
