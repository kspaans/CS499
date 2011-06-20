#ifndef SYSCALL_H
#define SYSCALL_H

#include <task.h>
#include <types.h>
#include <msg.h>

/* Create a new task with specified priority and entry point. */
int Create(int priority, void (*code)());
/* Identical to Create, but it makes a daemon task.
 * The kernel exits when no non-daemon tasks are running. */
int CreateDaemon(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Suspend();
void Exit() __attribute__((noreturn));

ssize_t sys_send(int chan, void *buf, size_t len, int  sch, int flags);
ssize_t sys_recv(int chan, void *buf, size_t len, int *rch, int flags);

int AwaitEvent(int eventid);
int TaskStat(int tid, struct task_stat *stat);

int ChannelOpen(void);
int ChannelClose(int no);
int ChannelDup(int oldfd, int newfd, int flags);

#endif /* SYSCALL_H */
