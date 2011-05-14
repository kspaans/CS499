#ifndef KERN_SYSCALL_H
#define KERN_SYSCALL_H

int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();
int Send(int tid, char *msg, int msglen, char *reply, int replylen);
int Receive(int *tid, char *msg, int msglen);
int Reply(int tid, char *reply, int replylen);

#endif /* KERN_SYSCALL_H */
