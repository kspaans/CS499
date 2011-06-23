#ifndef SYSCALL_H
#define SYSCALL_H

#include <task.h>
#include <types.h>
#include <msg.h>

#define SPAWN_DAEMON 1

int sys_spawn(int priority, void (*code)(void), int *chan, int chanlen, int flags);
int sys_gettid(void);
int sys_getptid(void);
void sys_yield(void);
void sys_suspend(void);
void sys_exit(void) __attribute__((noreturn));

int sys_channel(int flags);
int sys_close(int chan);
int sys_dup(int oldchan, int newchan, int flags);

ssize_t sys_send(int chan, const struct iovec *iov, int iovlen, int  sch, int flags);
ssize_t sys_recv(int chan, const struct iovec *iov, int iovlen, int *rch, int flags);

int sys_waitevent(int eventid);
int sys_taskstat(int tid, struct task_stat *stat);

// "wrappers" for userspace
static inline int spawn(int priority, void (*code)(void), int flags) {
	int default_chans[] = { 0, 1, 2 };
	return sys_spawn(priority, code, default_chans, arraysize(default_chans), flags);
}

#define gettid() sys_gettid()
#define getptid() sys_getptid()
#define yield() sys_yield()
#define suspend() sys_suspend()
#define exit() sys_exit()
#define channel(flags) sys_channel(flags)
#define close(chan) sys_close(chan)
#define dup(oldchan, newchan, flags) sys_dup(oldchan, newchan, flags)
#define send(chan, iov, iovlen, sch, flags) sys_send(chan, iov, iovlen, sch, flags)
#define recv(chan, iov, iovlen, rch, flags) sys_recv(chan, iov, iovlen, rch, flags)
#define waitevent(eventid) sys_waitevent(eventid)
#define taskstat(eventid) sys_taskstat(eventid)

#endif /* SYSCALL_H */
