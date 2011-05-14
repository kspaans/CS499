#include "message.h"
#include "lib.h"
#include "syscall.h"

int SendTyped(int tid, char *msg, int msglen, char *reply, int replylen, messagetype type) {
	char messagedata[sizeof(msg_t) + msglen];
	msg_t* message = (msg_t*)messagedata;
	message->type = type;
	memcpy(message->data, msg, msglen);
	return Send(tid, messagedata, sizeof(msg_t) + msglen, reply, replylen);
}


int ReplyTyped(int tid, char *msg, int msglen, messagetype type) {
	char messagedata[sizeof(msg_t) + msglen];
	msg_t* message = (msg_t*)messagedata;
	message->type = type;
	memcpy(message->data, msg, msglen);
	return Reply(tid, messagedata, sizeof(msg_t) + msglen);
}

