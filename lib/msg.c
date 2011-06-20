#include <types.h>
#include <string.h>
#include <errno.h>
#include <lib.h>
#include <syscall.h>
#include <kern/printk.h>

int MsgSend(int channel, int msgcode, const void *msg, int msglen, void *reply, int replylen, int *replychan) {
	int replych = ChannelOpen();

	int newlen = msglen + sizeof(msgcode);
	char newmsg[newlen];

	memcpy(&newmsg[0], &msgcode, sizeof(msgcode));
	memcpy(&newmsg[sizeof(msgcode)], msg, msglen); // do exactly what robert added this to avoid

	int status;
	int newreplen = replylen + sizeof(status);
	char newrep[newreplen];

	ssize_t ret = sys_send(channel, newmsg, newlen, replych, 0);
	if (ret < 0) {
		printk("MsgSend: send failed: %d", ret);
		Exit();
	}

	ret = sys_recv(replych, newrep, newreplen, replychan, 0);
	if (ret < 0) {
		printk("MsgSend: recv failed: %d", ret);
		Exit();
	}

	memcpy(&status, &newrep[0], sizeof(status));
	memcpy(reply, &newrep[sizeof(status)], replylen);

	ChannelClose(replych);

	return status;
}

int MsgReceive(int channel, int *tid, int *msgcode, void *msg, int msglen) {
	int replych;

	int newlen = msglen + sizeof(*msgcode);
	char newmsg[newlen];

	ssize_t ret = sys_recv(channel, newmsg, newlen, &replych, 0);
	if (ret < 0) {
		printk("MsgReceive: recv failed: (%d)", ret);
		Exit();
	}

	memcpy(msgcode, &newmsg[0], sizeof(msgcode));
	memcpy(msg, &newmsg[sizeof(msgcode)], msglen);

	*tid = replych; // haha
	return ret >= 4 ? ret - 4 : ret < 0 ? ret : EINVAL;
}

int MsgReply(int tid, int status, const void *reply, int replylen, int replychan) {
	int newlen = replylen + sizeof(status);
	int newmsg[newlen];

	memcpy(&newmsg[0], &status, sizeof(status));
	memcpy(&newmsg[sizeof(status)], reply, replylen);

	ssize_t ret = sys_send(tid, newmsg, newlen, replychan, 0);
	if (ret < 0) {
		printk("MsgReply: send failed (%d)", ret);
		Exit();
	}

	ChannelClose(tid); // haha
	return ret;
}

