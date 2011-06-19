#include <kern/syscallno.h>
@ Preprocessor workaround: # stringizes the argument, so it must be avoided.
#define SYSCALL(funcname, callno) \
	.global funcname; \
	.global kern_##funcname; \
	.type funcname, %function; \
funcname: \
	swi	callno; \
	bx	lr;

.text
.align	2

SYSCALL(Create, SYS_CREATE)
SYSCALL(CreateDaemon, SYS_CREATEDAEMON)
SYSCALL(MyTid, SYS_MYTID)
SYSCALL(MyParentTid, SYS_MYPTID)
SYSCALL(Pass, SYS_PASS)
SYSCALL(Exit, SYS_EXIT)
SYSCALL(MsgSend, SYS_MSGSEND)
SYSCALL(MsgReceive, SYS_MSGRECEIVE)
SYSCALL(MsgReply, SYS_MSGREPLY)
SYSCALL(MsgRead, SYS_MSGREAD)
SYSCALL(AwaitEvent, SYS_AWAITEVENT)
SYSCALL(TaskStat, SYS_TASKSTAT)
SYSCALL(Suspend, SYS_SUSPEND)
SYSCALL(ChannelOpen, SYS_CHANNELOPEN)
SYSCALL(ChannelClose, SYS_CHANNELCLOSE)
SYSCALL(ChannelDup, SYS_CHANNELDUP)
