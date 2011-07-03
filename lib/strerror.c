#include <errno.h>
#include <types.h>

static const char *errdesc[] = {
	[0] = "Success",
	[-ESTATE] = "Target task is in an invalid state",
	[-ENOSPC] = "Insufficient space in target buffer",
	[-EINVAL] = "Invalid request",
	[-ENOMEM] = "Out of memory",
	[-EINTR] = "Operation interrupted",
	[-ENOSYS] = "No such system call",
	[-ENOFUNC] = "Unimplemented or unsupported function",
	[-EMFILE] = "Too many open channels",
	[-EBADF] = "Bad channel descriptor",
	[-ESRCH] = "No such task",
	[-EBADMSG] = "Corrupt or illegal message",
	[-ENOENT] = "No such entity",
	[-EEXIST] = "Entity already exists",
	[-EBUSY] = "Resource busy",
	[-EWOULDBLOCK] = "Operation would block",
	[-EFAULT] = "Bad address",
};

const char *strerror(int errno) {
	if(errno > 0) {
		return "Success (>0)";
	} else if(errno <= -(int)arraysize(errdesc) || errdesc[-errno] == NULL) {
		return "Unknown or non-basic errno";
	} else {
		return errdesc[-errno];
	}
}
