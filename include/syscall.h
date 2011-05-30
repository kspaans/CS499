#ifndef SYSCALL_H
#define SYSCALL_H
#include <task.h>

/* Create a new task with specified priority and entry point. */
int Create(int priority, void (*code)());
/* Identical to Create, but it makes a daemon task.
 * The kernel exits when no non-daemon tasks are running. */
int CreateDaemon(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit() __attribute__((noreturn));
/* Send a message to "tid" of type "msgcode" and payload "msg"
 * Returns the status from Reply, or negative values on failure */
int Send(int tid, int msgcode, char *msg, int msglen, char *reply, int replylen);
/* Wait for a message. The size of the sent message is returned. */
int Receive(int *tid, int *msgcode, char *msg, int msglen);
/* Reply to a message. status will be returned as the return value from Send. */
int Reply(int tid, int status, char *reply, int replylen);
int AwaitEvent(int eventid);
int TaskStat(int tid, struct task_stat *stat);

#endif /* SYSCALL_H */
