#ifndef ERRNO_H
#define ERRNO_H

/* Error codes for system calls */
#define ERR_NOTID -1 /* non-existent or invalid TID */
#define ERR_STATE -2 /* target task is in the wrong state */
#define ERR_NOSPC -3 /* insufficient space in target task's buffer */
#define ERR_INVAL -4 /* invalid request */
#define ERR_NOMEM -5 /* not enough memory */
#define ERR_INTR  -6 /* transaction interrupted */
#define ERR_NOSYS -7 /* no such system call */
#define ERR_NOFUNC -8 /* unimplemented or nonexistent function */

#endif /* ERRNO_H */
