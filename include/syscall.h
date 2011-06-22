#ifndef SYSCALL_H
#define SYSCALL_H

#include <task.h>
#include <types.h>
#include <msg.h>

#define SPAWN_DAEMON 1

int sys_spawn(int priority, void (*code)(), int flags);
int sys_gettid();
int sys_getptid();
void sys_yield();
void sys_suspend();
void sys_exit() __attribute__((noreturn));

int sys_channel(int flags);
int sys_close(int chan);
int sys_dup(int oldchan, int newchan, int flags);

ssize_t sys_send(int chan, const struct iovec *iov, int iovlen, int  sch, int flags);
ssize_t sys_recv(int chan, const struct iovec *iov, int iovlen, int *rch, int flags);

int sys_waitevent(int eventid);
int sys_taskstat(int tid, struct task_stat *stat);

// "wrappers" for userspace
#define spawn(priority, code, flags) sys_spawn(priority, code, flags)
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
