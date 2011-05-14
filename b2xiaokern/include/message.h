#ifndef MESSAGE_H
#define MESSAGE_H

typedef enum {
	NAMESERVER_REGISTER,		/* msg_t: char name[<=64] */
	NAMESERVER_REGISTERREPLY,	/* int_msg_t: 0 on success, ERR_NAMESERVER_* on failure */
	NAMESERVER_WHOIS,			/* msg_t: char name[<=64] */
	NAMESERVER_WHOISREPLY,		/* int_msg_t: 0 on success, ERR_NAMESERVER_* on failure */
	NAMESERVER_GETNAME,			/* int_msg_t: tid to lookup */
	NAMESERVER_GETNAMEREPLY,	/* msg_t: char name[<=64] */
	NAMESERVER_INVALIDREQUEST,	/* int_msg_t: automatic failure message */

	RPS_SIGNUP,			/* msg_t: no payload */
	RPS_SIGNUPREPLY,	/* msg_t: no payload */
	RPS_PLAY,			/* int_msg_t: choice */
	RPS_PLAYREPLY,		/* int_msg_t: result */
	RPS_QUIT,			/* msg_t: no payload */
	RPS_QUITREPLY,		/* msg_t: no payload */
	RPS_INVALIDREQUEST,	/* msg_t: no payload */
} messagetype;

typedef struct {
	int type;
	char data[0];
} msg_t;

typedef struct {
	int type;
	int value;
} int_msg_t;

int SendTyped(int tid, char *msg, int msglen, char *reply, int replylen, messagetype type);
int ReplyTyped(int tid, char *msg, int msglen, messagetype type);

#endif
