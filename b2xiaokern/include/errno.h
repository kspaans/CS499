#ifndef KERN_ERRNO_H
#define KERN_ERRNO_H

/* Error codes for system calls */
#define ERR_CREATE_BADPRIO   -1 /* priority is invalid */
#define ERR_CREATE_NOMEM     -2 /* kernel is out of task descriptors */

#define ERR_SEND_BADTID      -1 /* task id is impossible */
#define ERR_SEND_NOSUCHTID   -2 /* task id is not an existing task */
#define ERR_SEND_SRRFAIL     -3 /* send-receive-reply transaction is incomplete */

#define ERR_RECEIVE_SRRFAIL  -1 /* [UNDOCUMENTED] temporary value to indicate that Receive has not yet returned */

#define ERR_REPLY_BADTID     -1 /* task id is impossible */
#define ERR_REPLY_NOSUCHTID  -2 /* task id is not an existing task */
#define ERR_REPLY_NOTBLOCKED -3 /* task is not reply blocked */
#define ERR_REPLY_NOSPACE    -4 /* insufficient space for the entire reply in sender's reply buffer */

#define ERR_NAMESERVER_BADTID -1 /* task id inside the wrapper is invalid */
#define ERR_NAMESERVER_NOTI -2 /* task id inside the wrapper is not the name server */
#define ERR_NAMESERVER_NOMEM -3 /* the name server is out of memory */
#define ERR_NAMESERVER_UNKNOWN -4 /* Unknown error */
#define ERR_NAMESERVER_NOSUCHTASK -5 /* No registered task with this name */
#define ERR_NAMESERVER_NAMETOOLONG -6 /* The name is too long */

#define ERR_BADCALL          -0xbeef

#endif /* KERN_ERRNO_H */
