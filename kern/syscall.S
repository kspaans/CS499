.set syscallno, 0
#define _SYSCALLNO(x) .set x, syscallno; .set syscallno, syscallno+1;
#include <kern/syscallno.h>

.macro syscall funcname callno
	.global \funcname
	.global kern_\funcname
	.type \funcname, %function
\funcname:
	mov     ip, $ \callno
	swi	0
	bx	lr
.endm

.text
.align	2

syscall sys_spawn SYS_SPAWN
syscall sys_gettid SYS_GETTID
syscall sys_getptid SYS_GETPTID
syscall sys_yield SYS_YIELD
syscall sys_exit SYS_EXIT
syscall sys_send SYS_SEND
syscall sys_recv SYS_RECV
syscall sys_event_wait SYS_EVENT_WAIT
syscall sys_taskstat SYS_TASKSTAT
syscall sys_suspend SYS_SUSPEND
syscall sys_channel SYS_CHANNEL
syscall sys_close SYS_CLOSE
syscall sys_chanflags SYS_CHANFLAGS
syscall sys_dup SYS_DUP
syscall sys_poll_add SYS_POLL_ADD
syscall sys_poll_remove SYS_POLL_REMOVE
syscall sys_poll_wait SYS_POLL_WAIT
