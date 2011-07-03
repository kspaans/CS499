#include <lib.h>
#include <string.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <panic.h>
#include <coproc.h>
#include <kern/task.h>
#include <kern/syscallno.h>
#include <kern/kmalloc.h>
#include <kern/printk.h>
#include <kern/backtrace.h>
#include <kern/ksyms.h>

static taskqueue freequeue;
static taskqueue taskqueues[TASK_NPRIO];
static taskqueue eventqueues[NEVENTS];

static int next_tidx;
static struct task *task_lookup[MAX_TASKS];

static int task_count;

/* visible to kernel main */
int nondaemon_count;

static struct channel *free_channel_head;

static void taskqueue_init(taskqueue *queue);
static void taskqueue_enqueue(taskqueue *queue, struct task *task);
static void task_move(struct task *task, taskqueue *queue);

void init_tasks(void) {
	int i;
	taskqueue_init(&freequeue);
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
static struct task *get_task(int tid) {
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

static int get_num_tasks(void) {
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
static struct task __attribute__((malloc)) *task_alloc(void) {
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

static void task_run(void (*code)(void)) {
	code();
	sys_exit();
}

static bool valid_channel(struct task *task, int chan) {
	return chan >= 0 && chan < MAX_TASK_CHANNELS && task->channels[chan].channel;
}

int syscall_spawn(struct task *task, int priority, void (*code)(void), int *chan, int chanlen, int flags) {
	int daemon = !!(flags & SPAWN_DAEMON);

	if (flags & ~(SPAWN_DAEMON))
		return EINVAL;
	if (priority < 0 || priority >= TASK_NPRIO)
		return EINVAL;
	if (chanlen < 0 || chanlen > MAX_TASK_CHANNELS)
		return EINVAL;
	for (int i = 0; i < chanlen; ++i)
		if (!valid_channel(task, chan[i]))
			return EBADF;

	struct task *newtask = task_alloc();

	if(newtask == NULL)
		return ENOMEM;

	if(!daemon)
		nondaemon_count++;
	task_count++;

	if(task)
		newtask->ptid = task->tid;
	else
		newtask->ptid = 0;
	newtask->priority = priority;
	newtask->daemon = daemon;
	newtask->state = TASK_RUNNING;

	memset(newtask->channels, 0, sizeof(newtask->channels));

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
		newtask->free_cd_head = -1;
		newtask->next_free_cd = chanlen;

		for (int i = 0; i < chanlen; ++i) {
			newtask->channels[i] = task->channels[chan[i]];
			newtask->channels[i].channel->refcount++;
		}
	} else {
		newtask->free_cd_head = -1;
		newtask->next_free_cd = 0;
	}

	set_task_running(newtask);

	//printk("created %p %d %s\n", newtask, newtask->tid, SYMBOL_EXACT(code));

	return newtask->tid;
}

static int syscall_gettid(struct task *task) {
	return task->tid;
}

static int syscall_getptid(struct task *task) {
	return get_ptid(task);
}

static void syscall_yield(struct task *task) {
	/* Do nothing. */
}

static int syscall_suspend(void) {
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

static int syscall_channel(struct task *task) {
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
	task->channels[no].flags = CHAN_RECV | CHAN_SEND;
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

static int syscall_close(struct task *task, int no) {
	if(no < 0 || no >= MAX_TASK_CHANNELS)
		return EBADF;
	if(task->channels[no].channel == NULL)
		return EBADF;

	close_channel(task, no);
	return 0;
}

static int syscall_chanflags(struct task *task, int fd) {
	if(fd < 0 || fd >= MAX_TASK_CHANNELS)
		return EBADF;
	if(task->channels[fd].channel == NULL)
		return EBADF;

	return task->channels[fd].flags;
}

static int syscall_dup(struct task *task, int oldfd, int newfd, int flags) {
	if(oldfd < 0 || oldfd >= MAX_TASK_CHANNELS)
		return EBADF;
	if(task->channels[oldfd].channel == NULL)
		return EBADF;

	if(oldfd == newfd) {
		task->channels[oldfd].flags = flags;
		return newfd;
	}

	if(newfd == -1) {
		newfd = alloc_channel_desc(task);
		if(newfd < 0)
			return EMFILE;
	} else if(newfd < 0 || newfd >= MAX_TASK_CHANNELS) {
		return EBADF;
	}

	if(task->channels[newfd].channel != NULL) {
		close_channel(task, newfd);
	}
	task->channels[oldfd].channel->refcount++;
	task->channels[newfd] = task->channels[oldfd];
	return newfd;
}

static void exit_task(struct task *task) {
	if(task->state != TASK_RUNNING)
		panic("Tried to exit non-running task!");
	if(!task->daemon)
		--nondaemon_count;
	--task_count;
	/* Close all channels. */
	for(int i=0; i<MAX_TASK_CHANNELS; ++i) {
		if(task->channels[i].channel != NULL) {
			close_channel(task, i);
		}
	}
	/* Task must have been running (not blocked or on the ready queue) to call sys_exit.
	   So, it is safe to free the task. */
	set_task_state(task, TASK_DEAD, &freequeue);
}

static void syscall_exit(struct task *task) {
	exit_task(task);
}

/* Destroy, if implemented, needs to do more work than sys_exit:
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

	if (sender->state != TASK_SEND_BLOCKED)
		panic("Task had a non-blocked task on send queue");
	if (receiver->state != TASK_RECV_BLOCKED)
		panic("Task had a non-blocked task on receive queue");

	size_t sendlen = iov_length(sender->sendbuf, sender->sendlen);
	size_t recvlen = iov_length(receiver->recvbuf, receiver->recvlen);

	// TODO: nasty case, for now I want this to be loud
	if (sendlen > recvlen)
		panic("message is too long");
	if (!sender->sendbuf || !receiver->recvbuf)
		panic("missing buffers");

	/* Copy the message */
	iov_copy(sender->sendbuf, sender->sendlen, receiver->recvbuf, receiver->recvlen);

	// transfer channel from sender to receiver
	if (sender->sendchan) {

		// TODO: decide what error this is
		if (!receiver->recvchan)
			panic("unwilling to receive channel");

		int cd = alloc_channel_desc(receiver);
		if (cd < 0)
			panic("out of channel descriptors");
		receiver->channels[cd].channel = sender->sendchan;
		receiver->channels[cd].flags = sender->sendchanflags;
		*receiver->recvchan = cd;
		sender->sendchan->refcount++;
	}

	// receiver unblocked, return length of message
	set_task_running(receiver);
	receiver->regs.r0 = sendlen;

	// sender unblocked, return success
	set_task_running(sender);
	sender->regs.r0 = 0;
}

/* Message passing */
static int syscall_send(struct task *task, int chan, const struct iovec *iov, int iovlen, int sch, int flags) {
	if (!valid_channel(task, chan))
		return EBADF;

	int chanflags = task->channels[chan].flags;
	struct channel *destchan = task->channels[chan].channel;
	if(!(chanflags & CHAN_SEND))
		return EBADF;
	bool nonblock = (chanflags & CHAN_NONBLOCK) || (flags & RECV_NONBLOCK);
	if(nonblock && taskqueue_empty(&destchan->receivers))
		return EWOULDBLOCK;

	int sendchanflags;
	struct channel *sendchan;
	if(sch != -1) {
		if(!valid_channel(task, sch))
			return EBADF;
		sendchanflags = task->channels[sch].flags;
		sendchan = task->channels[sch].channel;
	} else {
		sendchanflags = -1;
		sendchan = NULL;
	}

	set_task_state(task, TASK_SEND_BLOCKED, &destchan->senders);

	task->sendbuf = iov;
	task->sendlen = iovlen;
	task->destchan = destchan;
	task->sendchanflags = sendchanflags;
	task->sendchan = sendchan;

	handle_receive(destchan);

	return task->regs.r0;
}

static int syscall_recv(struct task *task, int chan, const struct iovec *iov, int iovlen, int *rch, int flags) {
	if (!valid_channel(task, chan))
		return EBADF;

	int chanflags = task->channels[chan].flags;
	struct channel *srcchan = task->channels[chan].channel;
	if(!(chanflags & CHAN_RECV))
		return EBADF;
	bool nonblock = (chanflags & CHAN_NONBLOCK) || (flags & RECV_NONBLOCK);
	if(nonblock && taskqueue_empty(&srcchan->senders))
		return EWOULDBLOCK;

	set_task_state(task, TASK_RECV_BLOCKED, &srcchan->receivers);

	task->recvbuf = iov;
	task->recvlen = iovlen;
	task->srcchan = srcchan;
	task->recvchan = rch;

	handle_receive(srcchan);

	return task->regs.r0;
}

static int syscall_poll_add(struct task *task, int chan, int flags) {
	return ENOSYS;
}

static int syscall_poll_remove(struct task *task, int chan, int flags) {
	return ENOSYS;
}

static int syscall_poll_wait(struct task *task, useraddr_t presult) {
	return ENOSYS;
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

static int syscall_event_wait(struct task *task, int eventid) {
	if(eventid < 0 || eventid >= NEVENTS)
		return EINVAL;

	set_task_state(task, TASK_EVENT_BLOCKED, &eventqueues[eventid]);
	task->eventid = eventid;

	// Return INTR... Real value will be posted when unblocked.
	return EINTR;
}

static int syscall_taskstat(struct task *task, int tid, useraddr_t stat) {
	struct task_stat st;
	if(tid == 0) {
		if(!stat)
			return get_num_tasks();
		st.tid = 0;
		st.ptid = 0;
		st.priority = 0;
		st.daemon = 0;
		st.state = TASK_RUNNING;
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
	}
	memcpy(stat, (void *)&st, sizeof(struct task_stat));
	return 0;
}

void task_syscall(struct task *task) {
	int ret;
	int code = task->regs.ip;
	switch (code) {
	case SYS_SPAWN:
		ret = syscall_spawn(task,
				(int)/* priority */task->regs.r0,
				(void *)/* code */task->regs.r1,
				(int *)/* chan */task->regs.r2,
				(int)/* chanlen */task->regs.r3,
				(int)/* flags */STACK_ARG(task, 4));
		break;
	case SYS_GETTID:
		ret = syscall_gettid(task);
		break;
	case SYS_GETPTID:
		ret = syscall_getptid(task);
		break;
	case SYS_YIELD:
		syscall_yield(task);
		ret = 0;
		break;
	case SYS_EXIT:
		syscall_exit(task);
		ret = 0;
		break;
	case SYS_SEND:
		ret = syscall_send(task,
			(int)/* chan */task->regs.r0,
			(void *)/* buf */task->regs.r1,
			(size_t)/* len */task->regs.r2,
			(int)/* sch */task->regs.r3,
			(int)/* flags */STACK_ARG(task, 4));
		break;
	case SYS_RECV:
		ret = syscall_recv(task,
			(int)/* chan */task->regs.r0,
			(void *)/* buf */task->regs.r1,
			(size_t)/* len */task->regs.r2,
			(int *)/* rch */task->regs.r3,
			(int)/* flags */STACK_ARG(task, 4));
		break;
	case SYS_EVENT_WAIT:
		ret = syscall_event_wait(task, task->regs.r0);
		break;
	case SYS_TASKSTAT:
		ret = syscall_taskstat(task, task->regs.r0, (useraddr_t)task->regs.r1);
		break;
	case SYS_SUSPEND:
		ret = syscall_suspend();
		break;
	case SYS_CHANNEL:
		ret = syscall_channel(task);
		break;
	case SYS_CLOSE:
		ret = syscall_close(task, task->regs.r0);
		break;
	case SYS_CHANFLAGS:
		ret = syscall_chanflags(task, task->regs.r0);
		break;
	case SYS_DUP:
		ret = syscall_dup(task, task->regs.r0, task->regs.r1, task->regs.r2);
		break;
	case SYS_POLL_ADD:
		ret = syscall_poll_add(task, task->regs.r0, task->regs.r1);
		break;
	case SYS_POLL_REMOVE:
		ret = syscall_poll_remove(task, task->regs.r0, task->regs.r1);
		break;
	case SYS_POLL_WAIT:
		ret = syscall_poll_wait(task, (useraddr_t)task->regs.r0);
		break;
	default:
		ret = ENOSYS;
	}
	task->regs.r0 = ret;
}

#define PRINT_REG(x) printk(#x ":%08x ", regs->x)

void print_regs(struct regs *regs) {
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

}

void print_task(struct task *task) {
	if(task == NULL)
		return;
	printk("task %p tid:%08x ptid:%08x prio:%d, state:%d, daemon:%d\n", task, task->tid, task->ptid, task->priority, task->state, task->daemon);
	print_regs(&task->regs);

	printk("stack pc:%08x(%s) lr:%08x(%s) ", task->regs.pc, SYMBOL(task->regs.pc),
	       task->regs.lr, SYMBOL(task->regs.lr));
	unwind_stack((void *)task->regs.fp);
	printk("\n");

	printk("\n");
}

void dump_tasks(void) {
	for (int i=0; i < next_tidx-1; ++i)
		print_task(task_lookup[i]);
}

void sysrq(void) {
	printk("SysRq\n");
	dump_tasks();
}

void task_dabt(struct task *task) {
	uint32_t dfar = read_coproc(p15, 0, c6, c0, 0);
	printk("Task Killed - Data Abort, address 0x%08x\n", dfar);
	print_task(task);
	exit_task(task);
}

void task_pabt(struct task *task) {
	uint32_t ifar = read_coproc(p15, 0, c6, c0, 2);
	printk("Task Killed - Prefetch Abort, address 0x%08x\n", ifar);
	print_task(task);
	exit_task(task);
}

void task_und(struct task *task) {
	printk("Task Killed - Undefined Instruction\n");
	print_task(task);
	exit_task(task);
}
