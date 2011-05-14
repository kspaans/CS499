#include "lib.h"
#include "errno.h"
#include "task.h"

/* _KernelEnd is defined by the linker in orex.ld */
extern int _KernelEnd;
static int mem_bottom;
static int mem_top;
static int next_tid;
static taskqueue taskqueues[TASK_NPRIO];

void init_tasks() {
	int i;
	/* Initialize task queues */
	for(i=0; i<TASK_NPRIO; i++) {
		taskqueue_init(&taskqueues[i]);
	}
	mem_bottom = (int)&_KernelEnd;
	/* minimum 256 MB installed on the board; leave a 4 MB gap for the kernel stack which starts at the top of memory */
	mem_top = 0x90000000 - (1<<22);
	next_tid = 1;
}

/* Cheap hack way to look up a TID.
 * Change this if TID allocation is changed. */
struct task *get_task(int tid) {
	if(tid <= 0) return NULL;
	if(tid >= next_tid) return NULL;
	return (struct task *)&_KernelEnd + (tid-1);
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
		task->prevtask = queue->end;
		queue->end = task;
	}
}

/* Allocate from the bottom of RAM. */
static void __attribute__((malloc)) *alloc_bottom(int size) {
	void *cur = (void *)mem_bottom;
	if(mem_bottom + size >= mem_top)
		return NULL;
	mem_bottom += size;
	return cur;
}

/* Allocate from the top of RAM. */
static void __attribute__((malloc)) *alloc_top(int size) {
	if(mem_top - size <= mem_bottom)
		return NULL;
	mem_top -= size;
	return (void *)mem_top;
}

int syscall_Create(struct task *task, int priority, void (*code)()) {
	if(priority < 0 || priority >= TASK_NPRIO)
		return ERR_CREATE_BADPRIO;

	/* TODO: When free lists are implemented, try the free lists instead of
	 * dying here. */
	struct task *newtask = alloc_bottom(sizeof(struct task));
	if(newtask == NULL)
		return ERR_CREATE_NOMEM;

	void *newtask_stack = alloc_top(TASK_STACKSIZE);
	if(newtask_stack == NULL)
		return ERR_CREATE_NOMEM;

	newtask->tid = next_tid++;
	newtask->parent = task;
	newtask->priority = priority;
	newtask->state = TASK_RUNNING;
	taskqueue_init(&(newtask->srr.recv_queue));

	/* Set up registers. */
	newtask->regs.pc = (int)code;
	newtask->regs.sp = (int)(newtask_stack) + TASK_STACKSIZE;
	newtask->regs.psr = get_user_psr();
	task_enqueue(newtask);
	return newtask->tid;
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
	/* Clean out the send queue. */
	struct task *sender;
	while(task->srr.recv_queue.start != NULL) {
		sender = taskqueue_pop(&(task->srr.recv_queue));
		if(sender->state != TASK_RECV_BLOCKED) {
			printf("KERNEL FATAL: Exiting task had a non-blocked task on receive queue.\n");
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
		printf("KERNEL FATAL: handle_receive called on a non-blocked task\n");
		return ERR_RECEIVE_SRRFAIL;
	}
	if(receiver->srr.recv_queue.start == NULL)
		return ERR_RECEIVE_SRRFAIL;
	struct task *sender = taskqueue_pop(&(receiver->srr.recv_queue));
	if(sender->state != TASK_RECV_BLOCKED) {
		printf("KERNEL FATAL: Task had a non-blocked task on receive queue.\n");
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

/// debug
#define PRINT_REG(x) printf(#x ":%08x ", task->regs.x)
void print_task(struct task *task) {
	printf("TASK PRINTOUT: %p\n", task);
	if(task == NULL) return;
	printf("REGS:\n");
	PRINT_REG(r0);
	PRINT_REG(r1);
	PRINT_REG(r2);
	PRINT_REG(r3);
	PRINT_REG(r4);
	PRINT_REG(r5);
	puts("");
	PRINT_REG(r6);
	PRINT_REG(r7);
	PRINT_REG(r8);
	PRINT_REG(r9);
	PRINT_REG(sl);
	PRINT_REG(fp);
	puts("");
	PRINT_REG(ip);
	PRINT_REG(sp);
	PRINT_REG(lr);
	PRINT_REG(pc);
	PRINT_REG(psr);
	puts("");
	printf("STATE:\n");
	printf("tid:%d parent:%p prio:%d, state:%d\n\n", task->tid, task->parent, task->priority, task->state);
}
/// debug
