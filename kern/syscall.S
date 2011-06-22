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

SYSCALL(sys_spawn, SYS_SPAWN)
SYSCALL(sys_gettid, SYS_GETTID)
SYSCALL(sys_getptid, SYS_GETPTID)
SYSCALL(sys_yield, SYS_YIELD)
SYSCALL(sys_exit, SYS_EXIT)
SYSCALL(sys_send, SYS_SEND)
SYSCALL(sys_recv, SYS_RECV)
SYSCALL(sys_waitevent, SYS_WAITEVENT)
SYSCALL(sys_taskstat, SYS_TASKSTAT)
SYSCALL(sys_suspend, SYS_SUSPEND)
SYSCALL(sys_channel, SYS_CHANNEL)
SYSCALL(sys_close, SYS_CLOSE)
SYSCALL(sys_dup, SYS_DUP)
