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

#define ERR_AWAITEVENT_INVALIDEVENT -1 /* eventid is invalid */
#define ERR_AWAITEVENT_CORRUPTEDDATA -2 /* corrupted volatile data */
#define ERR_AWAITEVENT_SRRFAIL -3 /* temporary value to indicate that AwaitEvent has not yet returned */

#define ERR_TASKSTAT_BADTID   -1 /* task id is impossible */

#define ERR_BADCALL          -0xbeef

#endif /* KERN_ERRNO_H */
