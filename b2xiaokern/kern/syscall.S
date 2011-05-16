#include "syscallno.h"
@ Preprocessor workaround: # stringizes the argument, so it must be avoided.
#define HASH #
#ifdef SUPERVISOR_TASKS
#define SYSCALL(funcname, callno) \
	.global funcname; \
	.global kern_##funcname; \
	.type funcname, %function; \
funcname: \
	stmfd sp!, {lr}; \
	swi	HASH callno; \
	ldmfd sp!, {lr}; \
	bx	lr;
#else
#define SYSCALL(funcname, callno) \
	.global funcname; \
	.global kern_##funcname; \
	.type funcname, %function; \
funcname: \
	swi	HASH callno; \
	bx	lr;
#endif

.text
.align	2

SYSCALL(Create, SYS_CREATE)
SYSCALL(CreateDaemon, SYS_CREATEDAEMON)
SYSCALL(MyTid, SYS_MYTID)
SYSCALL(MyParentTid, SYS_MYPTID)
SYSCALL(Pass, SYS_PASS)
SYSCALL(Exit, SYS_EXIT)
SYSCALL(Send, SYS_SEND)
SYSCALL(Receive, SYS_RECEIVE)
SYSCALL(Reply, SYS_REPLY)
SYSCALL(AwaitEvent, SYS_AWAITEVENT);
SYSCALL(TaskStat, SYS_TASKSTAT);
