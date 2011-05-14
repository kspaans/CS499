#include "syscallno.h"
@ Preprocessor workaround: # stringizes the argument, so it must be avoided.
#define HASH #
#define SYSCALL(funcname, callno) \
	.global funcname; \
	.global kern_##funcname; \
	.type funcname, %function; \
funcname: \
	swi	HASH callno; \
	bx	lr;

.text
.align	2

SYSCALL(Create, SYS_CREATE)
SYSCALL(MyTid, SYS_MYTID)
SYSCALL(MyParentTid, SYS_MYPTID)
SYSCALL(Pass, SYS_PASS)
SYSCALL(Exit, SYS_EXIT)
SYSCALL(Send, SYS_SEND)
SYSCALL(Receive, SYS_RECEIVE)
SYSCALL(Reply, SYS_REPLY)
