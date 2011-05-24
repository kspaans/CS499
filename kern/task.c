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
	taskqueue_init(&(newtask->srr.recv_queue));

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
	while(task->srr.recv_queue.start != NULL) {
		sender = taskqueue_pop(&(task->srr.recv_queue));
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

/* Handle a receive transaction; return the Receive call return value */
static int handle_receive(struct task *receiver) {
	if(receiver->state != TASK_SEND_BLOCKED) {
		printk("KERNEL FATAL: handle_receive called on a non-blocked task\n");
		return ERR_RECEIVE_SRRFAIL;
	}
	if(receiver->srr.recv_queue.start == NULL)
		return ERR_RECEIVE_SRRFAIL;
	struct task *sender = taskqueue_pop(&(receiver->srr.recv_queue));
	if(sender->state != TASK_RECV_BLOCKED) {
		printk("KERNEL FATAL: Task had a non-blocked task on receive queue.\n");
		return ERR_RECEIVE_SRRFAIL;
	}
	/* Copy the message. */
	if(receiver->srr.sendrecv_len < sender->srr.sendrecv_len) {
		memcpy(receiver->srr.sendrecv_buf, sender->srr.sendrecv_buf, receiver->srr.sendrecv_len);
	} else {
		memcpy(receiver->srr.sendrecv_buf, sender->srr.sendrecv_buf, sender->srr.sendrecv_len);
	}
	memcpy(receiver->srr.recv_tidptr, (void *)&(sender->tid), sizeof(int));
	sender->state = TASK_REPLY_BLOCKED;
	receiver->state = TASK_RUNNING;
	/* return the length of the sent message. Receiver needs to check that the buffer it provided was big enough. */
	return sender->srr.sendrecv_len;
}

/* Message passing */
int syscall_Send(struct task *task, int tid, useraddr_t msg, int msglen, useraddr_t reply, int replylen) {
	struct task *receiver = get_task(tid);
	if(receiver == NULL)
		return ERR_SEND_BADTID;
	if(receiver->tid != tid || receiver->state == TASK_DEAD)
		return ERR_SEND_NOSUCHTID;
	task->state = TASK_RECV_BLOCKED;
	task->srr.sendrecv_buf = msg;
	task->srr.sendrecv_len = msglen;
	task->srr.reply_buf = reply;
	task->srr.reply_len = replylen;
	task->srr.target_tid = tid;
	taskqueue_push(&(receiver->srr.recv_queue), task);
	/* Handle receiver */
	if(receiver->state == TASK_SEND_BLOCKED) {
		receiver->regs.r0 = handle_receive(receiver);
		/* and allow receiver to run, if they can */
		task_enqueue(receiver);
	}
	/* Return SRRFAIL here. Real return value will be posted by syscall_Reply. */
	return ERR_SEND_SRRFAIL;
}

int syscall_Receive(struct task *task, useraddr_t tid, useraddr_t msg, int msglen) {
	task->state = TASK_SEND_BLOCKED;
	task->srr.recv_tidptr = tid;
	task->srr.sendrecv_buf = msg;
	task->srr.sendrecv_len = msglen;
	return handle_receive(task);
}

int syscall_Reply(struct task *task, int tid, useraddr_t reply, int replylen) {
	struct task *sender = get_task(tid);
	if(sender == NULL)
		return ERR_REPLY_BADTID;
	if(sender->tid != tid || sender->state == TASK_DEAD)
		return ERR_REPLY_NOSUCHTID;
	if(sender->state != TASK_REPLY_BLOCKED)
		return ERR_REPLY_NOTBLOCKED;
	int ret;
	/* Copy the message. */
	if(sender->srr.reply_len < replylen) {
		memcpy(sender->srr.reply_buf, reply, sender->srr.reply_len);
		ret = ERR_REPLY_NOSPACE;
	} else {
		memcpy(sender->srr.reply_buf, reply, replylen);
		ret = 0;
	}
	/* Push sender return value and unblock it */
	sender->regs.r0 = replylen;
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
	task->srr.target_tid = eventid;
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
		} else {
			st.srrtid = othertask->srr.target_tid;
		}
	}
	memcpy(stat, (void *)&st, sizeof(struct task_stat));
	return 0;
}

void task_syscall(int code, struct task *task) {
	int ret;
	switch (code) {
	case SYS_CREATE:
		ret = syscall_Create(task, task->regs.r0, (void *) task->regs.r1);
		break;
	case SYS_CREATEDAEMON:
		ret = syscall_CreateDaemon(task, task->regs.r0, (void *) task->regs.r1);
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
		ret = syscall_Send(task, task->regs.r0, (useraddr_t) task->regs.r1,
				task->regs.r2, (useraddr_t) task->regs.r3, STACK_ARG(task, 5));
		break;
	case SYS_RECEIVE:
		ret = syscall_Receive(task, (useraddr_t) task->regs.r0,
				(useraddr_t) task->regs.r1, task->regs.r2);
		break;
	case SYS_REPLY:
		ret = syscall_Reply(task, task->regs.r0, (useraddr_t) task->regs.r1,
				task->regs.r2);
		break;
	case SYS_AWAITEVENT:
		ret = syscall_AwaitEvent(task, task->regs.r0);
		break;
	case SYS_TASKSTAT:
		ret = syscall_TaskStat(task, task->regs.r0, (useraddr_t) task->regs.r1);
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
