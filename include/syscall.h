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
void Suspend();
void Exit() __attribute__((noreturn));
/* Send a message to "tid" of type "msgcode" and payload "msg"
 * Returns the status from Reply, or negative values on failure */
int MsgSend(int tid, int msgcode, const void *msg, int msglen, void *reply, int replylen);
/* Wait for a message. The size of the sent message is returned. */
int MsgReceive(int *tid, int *msgcode, void *msg, int msglen);
/* Reply to a message. status will be returned as the return value from Send. */
int MsgReply(int tid, int status, const void *reply, int replylen);
#define MsgReplyStatus(tid,status) MsgReply(tid,status,NULL,0)
/* Read part or all of a received message. The number of bytes actually read is returned. */
int MsgRead(int tid, void *buf, int offset, int len);
/* Forward a received message to another task */
int MsgForward(int sender, int receiver, int msgcode);
int AwaitEvent(int eventid);
int TaskStat(int tid, struct task_stat *stat);

#endif /* SYSCALL_H */
