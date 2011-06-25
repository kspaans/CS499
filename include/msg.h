#ifndef MSG_H
#define MSG_H

/* Globally recognized message types */
enum msgcodes {
	STDOUT_WRITE_MSG,
	STDIN_GETCHAR_MSG,
	PRIVATE_MSG_START, /* start of private codes */
};

/* Send a message to "channel" of type "msgcode" and payload "msg"
 * Returns the status from Reply, or negative values on failure */
int MsgSend(int channel, int msgcode, const void *msg, int msglen, void *reply, int replylen, int *replychan);
/* Wait for a message. The size of the sent message is returned. */
int MsgReceive(int channel, int *tid, int *msgcode, void *msg, int msglen);
/* Reply to a message. status will be returned as the return value from Send. */
int MsgReply(int tid, int status, const void *reply, int replylen, int replychan);
#define MsgReplyStatus(tid,status) MsgReply(tid,status,NULL,0, -1)

#endif
