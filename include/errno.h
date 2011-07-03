#ifndef ERRNO_H
#define ERRNO_H

/* Error codes for system calls */
#define ESTATE -1 /* target task is in the wrong state */
#define ENOSPC -2 /* insufficient space in target task's buffer */
#define EINVAL -3 /* invalid request */
#define ENOMEM -4 /* not enough memory */
#define EINTR  -5 /* transaction interrupted */
#define ENOSYS -6 /* no such system call */
#define ENOFUNC -7 /* unimplemented or nonexistent function */
#define EMFILE -8
#define EBADF -9
#define ESRCH -10
#define EBADMSG -11
#define ENOENT -12
#define EEXIST -13
#define EBUSY -14
#define EWOULDBLOCK -15
#define EFAULT -16

#endif /* ERRNO_H */
