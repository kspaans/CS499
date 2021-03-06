#ifndef KERN_TASK_H
#define KERN_TASK_H

#include <task.h>
#include <syscall.h>

typedef void* useraddr_t;
typedef const void* const_useraddr_t;

/* Number of non-daemon tasks. The system exits when no non-daemon tasks remain. */
extern int nondaemon_count;

/* Task index and generation */
#define TID_IDX(tid) ((tid) & 0xfff)
#define TID_GEN(tid) ((tid) >> 12)
#define MAKE_TID(idx, gen) (((gen) << 12) | (idx))

/* Number of priorities supported.
 * Priority numbers run from 0 (highest priority)
 * to TASK_NPRIO-1 (lowest priority). */
#define TASK_NPRIO          8

#define MAX_TASK_CHANNELS 64

/* Size of the stack allocated to each task */
#define TASK_STACKSIZE      65536 /* 64 KB */

#define STACK_ARG(task, n)  (*(((int *)task->regs.sp) + (n-4)))

/* NOTE: Changes to the regs stucture MUST be reflected in switch.S. */
struct regs {
	/* The layout of this struct is due to the construction of switch.S.
	   See that file for the gory details. */
	int r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr, pc;
	int psr;
};

/* Forward declaration */
struct task;
typedef struct {
	struct task *start;
} taskqueue;

struct cdnode {
	struct cdnode *next, *prev;
	struct channel_desc *cd;
};

struct channel {
	int refcount; /* Number of open channel descriptors to this channel */
	taskqueue senders; /* Queue of tasks send-blocked on this channel */
	taskqueue receivers; /* Queue of tasks receive-blocked on this channel */
	struct cdnode poll_list[POLL_NEVENTS]; /* Circular list of channel descriptors polling on this channel */
	struct channel *next_free_channel; /* Channel free list */
};

struct channel_desc {
	int flags;
	struct task *task;
	struct channel *channel;
	/* Channel descriptors are on at most one list per event at any given time:
	   they are either on the channel poll list, the task poll queue, or no
	   list at all. */
	struct cdnode poll_nodes[POLL_NEVENTS];
	int next_free_cd;
};

struct task {
	/* regs must be first */
	struct regs regs;
	int tid;
	int ptid;
	int priority;
	int daemon;
	int stack_start;
	enum taskstate state;

	int free_cd_head;
	int next_free_cd;
	struct channel_desc channels[MAX_TASK_CHANNELS];
	struct cdnode poll_queue[POLL_NEVENTS]; /* Circular list of channel descriptors which have reported an event */
	int poll_count; /* Number of events polled by this task */

	union {
		struct {
			const struct iovec *sendbuf;
			int sendlen;
			struct channel *destchan;
			int sendchanflags;
			struct channel *sendchan;
		};
		struct {
			const struct iovec *recvbuf;
			int recvlen;
			struct channel *srcchan;
			int *recvchan;
		};
		struct pollresult *pollresult;
		int eventid;
	};

	/* for putting tasks in queues */
	taskqueue *queue;
	struct task *next;
	struct task *prev;
};

/* Initialize task system */
void init_tasks(void);
/* Get the initial PSR for user functions */
int get_user_psr(void); /* asm function */
/* Activate the specified task, allowing it to run */
void task_activate(struct task *task); /* asm function */
/* SWI and IRQ handlers */
void task_syscall(struct task *task); /* in task.c */
void task_irq(void); /* in interrupt.c */
/* Check for stack overflow */
void check_stack(struct task* task);

struct task *schedule(void);

/// debug
void print_task(struct task *task);
/// debug

/* System calls */
int syscall_spawn(struct task *task, int priority, void (*code)(void), int *chan, int chanlen, int flags);

void event_unblock_all(int eventid, int return_value);
void event_unblock_one(int eventid, int return_value);

void cpu_info(void);

void sysrq(char cmd);

int main(void);

void kernel_und(struct regs *regs);
void kernel_pabt(struct regs *regs);
void kernel_dabt(struct regs *regs);
void kernel_irq(struct regs *regs);

void task_und(struct task *task);
void task_pabt(struct task *task);
void task_dabt(struct task *task);

void print_regs(struct regs *regs);
void dump_tasks(void);

#endif /* KERN_TASK_H */
