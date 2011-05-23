#ifndef SYSCALL_H
#define SYSCALL_H
#include <task.h>

int Create(int priority, void (*code)());
/* Identical to Create, but it makes a daemon task.
 * The kernel exits when no non-daemon tasks are running. */
int CreateDaemon(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit() __attribute__((noreturn));
int Send(int tid, char *msg, int msglen, char *reply, int replylen);
int Receive(int *tid, char *msg, int msglen);
int Reply(int tid, char *reply, int replylen);
int AwaitEvent(int eventid);
int TaskStat(int tid, struct task_stat *stat);

#endif /* SYSCALL_H */
