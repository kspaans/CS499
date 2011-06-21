#include <types.h>
#include <string.h>
#include <errno.h>
#include <lib.h>
#include <syscall.h>
#include <kern/printk.h>

int MsgSend(int channel, int msgcode, const void *msg, int msglen, void *reply, int replylen, int *replychan) {
	int replych = ChannelOpen();
	int status;

	struct iovec send_iov[] = {
		{ &msgcode, sizeof(msgcode) },
		{ (void *)msg, msglen },
	};

	struct iovec recv_iov[] = {
		{ &status, sizeof(status) },
		{ reply, replylen },
	};

	ssize_t ret = sys_send(channel, send_iov, arraysize(send_iov), replych, 0);
	if (ret < 0) {
		printk("MsgSend: send failed: %d", ret);
		Exit();
	}

	ret = sys_recv(replych, recv_iov, arraysize(recv_iov), replychan, 0);
	if (ret < 0) {
		printk("MsgSend: recv failed: %d", ret);
		Exit();
	}

	ChannelClose(replych);

	return status;
}

int MsgReceive(int channel, int *tid, int *msgcode, void *msg, int msglen) {
	int replych;

	struct iovec recv_iov[] = {
		{ msgcode, sizeof(*msgcode) },
		{ msg, msglen },
	};

	ssize_t ret = sys_recv(channel, recv_iov, arraysize(recv_iov), &replych, 0);
	if (ret < 0) {
		printk("MsgReceive: recv failed: (%d)", ret);
		Exit();
	}

	*tid = replych; // haha

	if (ret >= 0 && ret < 4)
		panic("wrong number of bytes in receive");

	return ret >= 4 ? ret - 4 : ret;
}

int MsgReply(int tid, int status, const void *reply, int replylen, int replychan) {
	struct iovec reply_iov[] = {
		{ &status, sizeof(status) },
		{ (void *)reply, replylen },
	};

	ssize_t ret = sys_send(tid, reply_iov, arraysize(reply_iov), replychan, 0);
	if (ret < 0) {
		printk("MsgReply: send failed (%d)", ret);
		Exit();
	}

	ChannelClose(tid); // haha
	return ret;
}
