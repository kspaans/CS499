#include <lib.h>
#include <string.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <kern/task.h>
#include <kern/syscallno.h>
#include <kern/kmalloc.h>
#include <kern/printk.h>
#include <kern/backtrace.h>
#include <kern/ksyms.h>

static taskqueue freequeue;
static taskqueue taskqueues[TASK_NPRIO];
static taskqueue eventqueues[NEVENTS];
static taskqueue replyqueue;

static int next_tidx;
static struct task *task_lookup[MAX_TASKS];

static int task_count;

/* visible to kernel main */
int nondaemon_count;

static struct channel *free_channel_head;

static void taskqueue_init(taskqueue *queue);
static void taskqueue_enqueue(taskqueue *queue, struct task *task);
static void task_move(struct task *task, taskqueue *queue);

void init_tasks() {
	int i;
	taskqueue_init(&freequeue);
	taskqueue_init(&replyqueue);
	/* Initialize task queues */
	for(i=0; i<TASK_NPRIO; i++) {
		taskqueue_init(&taskqueues[i]);
	}
	for(i=0; i<NEVENTS; i++) {
		taskqueue_init(&eventqueues[i]);
	}

	next_tidx = 1;
	task_count = 0;
	nondaemon_count = 0;
	free_channel_head = NULL;
}

/* Change this if TID allocation is changed. */
struct task *get_task(int tid) {
	int idx = TID_IDX(tid);
	if(idx <= 0) return NULL;
	if(idx >= next_tidx) return NULL;

	struct task *ret = task_lookup[idx-1];
	if(ret->tid != tid) return NULL;
	return ret;
}

static int get_ptid(struct task *task) {
	if(get_task(task->ptid))
		return task->ptid;
	task->ptid = 0;
	return 0;
}

int get_num_tasks() {
	return task_count;
}

void check_stack(struct task* task) {
	if(task->regs.sp<task->stack_start) {
		printk("Error: Task %d has overflowed its stack.\n", task->tid);
	}
}

static void taskqueue_init(taskqueue* queue) {
	queue->start = NULL;
}

static bool taskqueue_empty(taskqueue *queue) {
	return queue->start == NULL;
}

static struct task *taskqueue_advance(taskqueue *queue) {
	struct task *ret = queue->start;
	queue->start = queue->start->next;
	return ret;
}

static struct task *taskqueue_peek(taskqueue *queue) {
	return queue->start;
}

struct task *schedule(void) {
	for (int i = 0; i < TASK_NPRIO; ++i)
		if (!taskqueue_empty(&taskqueues[i]))
			return taskqueue_advance(&taskqueues[i]);

	panic("no task to schedule");
}

static void taskqueue_enqueue(taskqueue *queue, struct task *task) {
	if (taskqueue_empty(queue)) {
		queue->start = task;
		task->next = task;
		task->prev = task;
	} else {
		struct task *next = queue->start;
		struct task *prev = next->prev;
		task->next = next;
		task->prev = prev;
		prev->next = task;
		next->prev = task;
	}
	task->queue = queue;
}

static void taskqueue_dequeue(taskqueue *queue, struct task *task) {
	if (task->queue->start == task)
		task->queue->start = task->next;

	task->next->prev = task->prev;
	task->prev->next = task->next;

	if (task->next == task)
		task->queue->start = NULL;

	task->queue = NULL;
}

static void task_move(struct task *task, taskqueue *queue) {
	taskqueue_dequeue(task->queue, task);
	taskqueue_enqueue(queue, task);
}

static void set_task_state(struct task *task, int state, taskqueue *queue) {
	task->state = state;
	task_move(task, queue);
}

static void set_task_running(struct task *task) {
	set_task_state(task, TASK_RUNNING, &taskqueues[task->priority]);
}

/* Allocate a task and add it to the lookup list. */
static struct task __attribute__((malloc)) *task_alloc() {
	struct task *ret;

	if (taskqueue_empty(&freequeue)) {
		if(next_tidx >= MAX_TASKS)
			return NULL;

		ret = kmalloc(sizeof(struct task));
		if(ret == NULL)
			return NULL;

		void *stack = kmalloc(TASK_STACKSIZE);
		if(stack == NULL)
			return NULL;

		ret->stack_start = (int)stack;

		int tidx = next_tidx++;
		ret->tid = MAKE_TID(tidx, 0);

		ret->queue = NULL;

		taskqueue_enqueue(&freequeue, ret);
	} else {
		ret = taskqueue_peek(&freequeue);
		ret->tid = MAKE_TID(TID_IDX(ret->tid), TID_GEN(ret->tid)+1);
	}

	task_lookup[TID_IDX(ret->tid)-1] = ret;
	return ret;
}

static void __attribute__((noreturn)) task_run(void (*code)()) {
	code();
	Exit();
}

static int do_Create(struct task *task, int priority, void (*code)(), int daemon) {
	if(priority < 0 || priority >= TASK_NPRIO)
		return EINVAL;

	struct task *newtask = task_alloc();

	if(newtask == NULL)
		return ENOMEM;

	if(daemon != TASK_DAEMON)
		nondaemon_count++;
	task_count++;

	if(task)
		newtask->ptid = task->tid;
	else
		newtask->ptid = 0;
	newtask->priority = priority;
	newtask->daemon = (daemon == TASK_DAEMON);
	newtask->state = TASK_RUNNING;

	/* Set up registers. */
	newtask->regs.pc = (int)task_run;
	newtask->regs.psr = get_user_psr();
	newtask->regs.r0 = (int)code;
	newtask->regs.sp = newtask->stack_start + TASK_STACKSIZE;
	/* set fp and lr to 0 to make backtrace happy */
	newtask->regs.fp = 0;
	newtask->regs.lr = 0;

	/* duplicate open channels */
	if(task) {
		newtask->free_cd_head = task->free_cd_head;
		newtask->next_free_cd = task->next_free_cd;
		for(int i=0; i<MAX_TASK_CHANNELS; ++i) {
			newtask->channels[i] = task->channels[i];
			if(task->channels[i].channel != NULL) {
				task->channels[i].channel->refcount++;
			}
		}
	} else {
		newtask->free_cd_head = -1;
		newtask->next_free_cd = 0;
		memset(newtask->channels, 0, sizeof(newtask->channels));
	}

	set_task_running(newtask);

	printk("created %p %d %s\n", newtask, newtask->tid, SYMBOL_EXACT(code));

	return newtask->tid;
}

int KernCreateTask(int priority, void (*code)(), int daemon) {
	return do_Create(NULL, priority, code, daemon);
}

int syscall_Create(struct task *task, int priority, void (*code)()) {
	return do_Create(task, priority, code, TASK_NORMAL); // create non-daemon task
}

int syscall_CreateDaemon(struct task *task, int priority, void (*code)()) {
	return do_Create(task, priority, code, TASK_DAEMON); // create daemon task
}

int syscall_MyTid(struct task *task) {
	return task->tid;
}

int syscall_MyParentTid(struct task *task) {
	return get_ptid(task);
}

void syscall_Pass(struct task *task) {
	/* Do nothing. */
}

int syscall_Suspend(void) {
	// TODO: this technically can finish without an interrupt occuring (if not implemented)
	asm("wfi");
	task_irq();
	return 0;
}

static void free_channel_desc(struct task *task, int no) {
	task->channels[no].next_free_cd = task->free_cd_head;
	task->free_cd_head = no;
}

static int alloc_channel_desc(struct task *task) {
	int no;
	if(task->free_cd_head < 0) {
		if(task->next_free_cd >= MAX_TASK_CHANNELS)
			return -1;
		no = task->next_free_cd++;
	} else {
		no = task->free_cd_head;
		task->free_cd_head = task->channels[no].next_free_cd;
	}
	return no;
}

int syscall_ChannelOpen(struct task *task) {
	int no = alloc_channel_desc(task);
	if (no < 0)
		return EMFILE;
	struct channel *channel;
	if(free_channel_head == NULL) {
		channel = kmalloc(sizeof(*channel));
	} else {
		channel = free_channel_head;
		free_channel_head = channel->next_free_channel;
	}
	memset(channel, 0, sizeof(*channel));
	channel->refcount++;
	taskqueue_init(&channel->senders);
	taskqueue_init(&channel->receivers);
	task->channels[no].channel = channel;
	return no;
}

static void close_channel(struct task *task, int no) {
	struct channel *channel = task->channels[no].channel;
	task->channels[no].channel = NULL;
	free_channel_desc(task, no);

	channel->refcount--;
	if(channel->refcount == 0) {
		if(!taskqueue_empty(&channel->receivers))
			panic("Invalid refcount on channel: receivers still exist");
		if(channel->senders.start != NULL)
			panic("Invalid refcount on channel: senders still exist");
		channel->next_free_channel = free_channel_head;
		free_channel_head = channel;
	}
}

int syscall_ChannelClose(struct task *task, int no) {
	if(no < 0 || no >= MAX_TASK_CHANNELS)
		return EBADF;

	if(task->channels[no].channel == NULL)
		return EBADF;

	close_channel(task, no);
	return 0;
}

int syscall_ChannelDup(struct task *task, int oldfd, int newfd, int flags) {
	if(oldfd < 0 || oldfd >= MAX_TASK_CHANNELS)
		return EBADF;
	if(task->channels[oldfd].channel == NULL)
		return EBADF;
	if(oldfd == newfd) {
		task->channels[oldfd].flags = flags;
		return 0;
	}

	if(newfd < 0 || newfd >= MAX_TASK_CHANNELS)
		return EBADF;
	if(task->channels[newfd].channel != NULL) {
		close_channel(task, newfd);
	}
	task->channels[oldfd].channel->refcount++;
	task->channels[newfd] = task->channels[oldfd];
	return 0;
}

void syscall_Exit(struct task *task) {
	if(!task->daemon)
		--nondaemon_count;
	--task_count;
	/* Close all channels. */
	for(int i=0; i<MAX_TASK_CHANNELS; ++i) {
		if(task->channels[i].channel != NULL) {
			close_channel(task, i);
		}
	}
	/* Task must have been running (not blocked or on the ready queue) to call Exit.
	   So, it is safe to free the task. */
	set_task_state(task, TASK_DEAD, &freequeue);
}

/* Destroy, if implemented, needs to do more work than Exit:
 * - Destroy must remove tasks from the queue that they are on.
 *   - This requires a task to know which queue it is on.
 */

/* Handle a receive transaction, unblocking the receiver. */
static void handle_receive(struct channel *chan) {
	if (taskqueue_empty(&chan->receivers))
		return;
	if (taskqueue_empty(&chan->senders))
		return;

	struct task *sender = taskqueue_peek(&chan->senders);
	struct task *receiver = taskqueue_peek(&chan->receivers);

	if (sender->state != TASK_RECV_BLOCKED)
		panic("Task had a non-blocked task on send queue");
	if (receiver->state != TASK_SEND_BLOCKED)
		panic("Task had a non-blocked task on receive queue");

	/* Copy the message */
	if(receiver->srr.recv.buf && sender->srr.send.buf) {
		int len;
		if(receiver->srr.recv.len < sender->srr.send.len)
			len = receiver->srr.recv.len;
		else
			len = sender->srr.send.len;
		memcpy(receiver->srr.recv.buf, sender->srr.send.buf, len);
	}

	/* Copy the status data */
	if(receiver->srr.recv.tidptr)
		memcpy(receiver->srr.recv.tidptr, (void *)&sender->tid, sizeof(int));
	if(receiver->srr.recv.codeptr)
		memcpy(receiver->srr.recv.codeptr, (void *)&sender->srr.send.code, sizeof(int));

	set_task_state(sender, TASK_REPLY_BLOCKED, &replyqueue);

	// return full send length; receiver should check for truncation
	receiver->regs.r0 = sender->srr.send.len;
	set_task_running(receiver);
}

/* Message passing */
int syscall_MsgSend(struct task *sender, int channel, int msgcode, const_useraddr_t msg, int msglen, useraddr_t reply, int replylen, int *replychan) {
	if (channel < 0 || channel >= MAX_TASK_CHANNELS)
		return EINVAL;
	if (!sender->channels[channel].channel)
		return EBADF;
	struct channel *chan = sender->channels[channel].channel;

	set_task_state(sender, TASK_RECV_BLOCKED, &chan->senders);

	sender->srr.send.channel = channel;
	sender->srr.send.code = msgcode;
	sender->srr.send.buf = msg;
	sender->srr.send.len = msglen;
	sender->srr.send.rbuf = reply;
	sender->srr.send.rlen = replylen;
	sender->srr.send.rchan = replychan;

	handle_receive(chan);

	/* Return INTR here. Real return value will be posted by syscall_Reply. */
	return EINTR;
}

int syscall_MsgReceive(struct task *receiver, int channel, useraddr_t tid, useraddr_t msgcode, useraddr_t msg, int msglen) {
	if (channel < 0 || channel >= MAX_TASK_CHANNELS)
		return EINVAL;
	if (!receiver->channels[channel].channel)
		return EBADF;
	struct channel *chan = receiver->channels[channel].channel;

	set_task_state(receiver, TASK_SEND_BLOCKED, &chan->receivers);

	receiver->srr.recv.tidptr = tid;
	receiver->srr.recv.codeptr = msgcode;
	receiver->srr.recv.buf = msg;
	receiver->srr.recv.len = msglen;

	handle_receive(chan);

	return receiver->regs.r0;
}

int syscall_MsgReply(struct task *task, int tid, int status, const_useraddr_t reply, int replylen, int replychan) {
	struct task *sender = get_task(tid);
	if(sender == NULL || sender->state == TASK_DEAD)
		return ESRCH;
	if(sender->state != TASK_REPLY_BLOCKED)
		return ESTATE;
	if(replychan >= MAX_TASK_CHANNELS || replychan < -1)
		return EINVAL;

	struct channel_desc *channel = NULL;
	if(replychan >= 0) {
		if (!task->channels[replychan].channel)
			return EBADF;
		channel = &task->channels[replychan];
	}

	/* ret is returned to the receiver (this task),
	   status is returned to the sender (other task) */
	int ret = 0;
	/* Copy the message. */
	if(sender->srr.send.rbuf && reply) {
		if(sender->srr.send.rlen < replylen) {
			ret = status = ENOSPC;
			goto out;
		} else {
			memcpy(sender->srr.send.rbuf, reply, replylen);
		}
	}
	if (sender->srr.send.rchan) {
		if (!channel) {
			*sender->srr.send.rchan = -1;
			goto out;
		}

		int cd = alloc_channel_desc(sender);
		if (cd < 0) {
			status = EMFILE;
			goto out;
		}

		channel->channel->refcount++;
		sender->channels[cd] = *channel;
		*sender->srr.send.rchan = cd;
	}

out:
	/* Push sender return value and unblock it */
	sender->regs.r0 = status;
	set_task_running(sender);
	return ret;
}

int syscall_MsgRead(struct task *task, int tid, useraddr_t buf, int offset, int len) {
	struct task *sender = get_task(tid);
	if(sender == NULL || sender->state == TASK_DEAD)
		return ESRCH;
	if(sender->state != TASK_REPLY_BLOCKED)
		return ESTATE;

	if(sender->srr.send.buf == NULL)
		return 0;

	if(len > sender->srr.send.len - offset)
		len = sender->srr.send.len - offset;

	memcpy(buf, (char *)sender->srr.send.buf + offset, len);
	return len;
}

void event_unblock_all(int eventid, int return_value) {
	while (!taskqueue_empty(&eventqueues[eventid]))
		event_unblock_one(eventid, return_value);
}

void event_unblock_one(int eventid, int return_value) {
	if (!taskqueue_empty(&eventqueues[eventid])) {
		struct task *task = taskqueue_advance(&eventqueues[eventid]);
		task->regs.r0 = return_value;
		set_task_running(task);
	}
}

int syscall_AwaitEvent(struct task *task, int eventid) {
	if(eventid < 0 || eventid >= NEVENTS)
		return EINVAL;

	set_task_state(task, TASK_EVENT_BLOCKED, &eventqueues[eventid]);
	task->srr.event.id = eventid;

	// Return INTR... Real value will be posted when unblocked.
	return EINTR;
}

int syscall_TaskStat(struct task *task, int tid, useraddr_t stat) {
	struct task_stat st;
	if(tid == 0) {
		if(!stat)
			return get_num_tasks();
		st.tid = 0;
		st.ptid = 0;
		st.priority = 0;
		st.daemon = 0;
		st.state = TASK_RUNNING;
		st.srrtid = -1;
	} else {
		struct task *othertask = get_task(tid);
		if(othertask == NULL)
			return ESRCH;
		if(!stat)
			return 0;
		st.tid = othertask->tid;
		st.ptid = get_ptid(task);
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
	case SYS_MSGSEND:
		ret = syscall_MsgSend(task, /*tid*/task->regs.r0, /*msgcode*/task->regs.r1,
			/*msg*/(useraddr_t)task->regs.r2, /*msglen*/task->regs.r3,
			/*reply*/(useraddr_t)STACK_ARG(task, 4), /*replylen*/STACK_ARG(task, 5),
			/*replychan*/(useraddr_t)STACK_ARG(task, 6));
		break;
	case SYS_MSGRECEIVE:
		ret = syscall_MsgReceive(task, /*channel*/task->regs.r0,
			/*tid*/(useraddr_t)task->regs.r1,
			/*msgcode*/(useraddr_t)task->regs.r2, /*msg*/(useraddr_t)task->regs.r3,
			/*msglen*/STACK_ARG(task, 4));
		break;
	case SYS_MSGREPLY:
		ret = syscall_MsgReply(task, /*tid*/task->regs.r0, /*status*/task->regs.r1,
			/*reply*/(useraddr_t)task->regs.r2, /*replylen*/task->regs.r3,
			/*replychan*/(int)STACK_ARG(task, 4));
		break;
	case SYS_MSGREAD:
		ret = syscall_MsgRead(task, /*tid*/task->regs.r0, /*buf*/(useraddr_t)task->regs.r1,
			/*offset*/task->regs.r2, /*len*/task->regs.r3);
		break;
	case SYS_AWAITEVENT:
		ret = syscall_AwaitEvent(task, task->regs.r0);
		break;
	case SYS_TASKSTAT:
		ret = syscall_TaskStat(task, task->regs.r0, (useraddr_t)task->regs.r1);
		break;
	case SYS_SUSPEND:
		ret = syscall_Suspend();
		break;
	case SYS_CHANNELOPEN:
		ret = syscall_ChannelOpen(task);
		break;
	case SYS_CHANNELCLOSE:
		ret = syscall_ChannelClose(task, task->regs.r0);
		break;
	case SYS_CHANNELDUP:
		ret = syscall_ChannelDup(task, task->regs.r0, task->regs.r1, task->regs.r2);
		break;
	default:
		ret = ENOSYS;
	}
	task->regs.r0 = ret;
}

#define PRINT_REG(x) printk(#x ":%08x ", task->regs.x)
void print_task(struct task *task) {
	if(task == NULL)
		return;
	printk("task %p tid:%08x ptid:%08x prio:%d, state:%d\n", task, task->tid, task->ptid, task->priority, task->state);
	PRINT_REG(r0);
	PRINT_REG(r1);
	PRINT_REG(r2);
	PRINT_REG(r3);
	PRINT_REG(r4);
	PRINT_REG(r5);
	printk("\n");
	PRINT_REG(r6);
	PRINT_REG(r7);
	PRINT_REG(r8);
	PRINT_REG(r9);
	PRINT_REG(sl);
	PRINT_REG(fp);
	printk("\n");
	PRINT_REG(ip);
	PRINT_REG(sp);
	PRINT_REG(lr);
	PRINT_REG(pc);
	PRINT_REG(psr);
	printk("\n");

	printk("stack pc:%08x(%s) lr:%08x(%s) ", task->regs.pc, SYMBOL(task->regs.pc),
	       task->regs.lr, SYMBOL(task->regs.lr));
	unwind_stack((void *)task->regs.fp);
	printk("\n");

	printk("\n");
}

void sysrq(void) {
	printk("SysRq\n");
	for (int i=0; i < next_tidx-1; ++i) {
		print_task(task_lookup[i]);
	}
}
