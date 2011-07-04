#include <errno.h>
#include <lib.h>
#include <syscall.h>
#include <servers/fs.h>
#include <servers/lock.h>

/* Lock server. This is a pretty massive abuse of the channel system...
 * don't tell anyone :P */
enum lockmsg {
	LOCK_REGISTER_MSG = PRIVATE_MSG_START,
	LOCK_UNREGISTER_MSG,
};

/* Emulate a MsgSend, but substitute the lock channel for the reply channel */
int lockchan_register(int chan) {
	int fd = open(ROOT_DIRFD, "/services/lock");
	if(fd < 0)
		return fd;

	int msgcode = LOCK_REGISTER_MSG;
	struct iovec iov[] = {
		{ &msgcode, sizeof(msgcode) },
	};

	int ret = send(fd, iov, arraysize(iov), chan, 0);
	if(ret < 0)
		return ret;

	ret = recv(chan, iov, arraysize(iov), NULL, 0);
	if(ret < 0)
		return ret;

	return msgcode;
}

int lockchan_unregister(int chan) {
	return sendpath("/services/lock", LOCK_UNREGISTER_MSG, &chan, sizeof(chan), NULL, 0);
}

/* locking == sending, unlocking == receiving.
 * An "unlocking" task can directly unlock (receive) a "locking" task,
 * without having to go through the lockserver first. This actually works. */
int lock_channel(int chan) {
	return send(chan, NULL, 0, -1, 0);
}

int unlock_channel(int chan) {
	return recv(chan, NULL, 0, NULL, 0);
}

static void lockserver_task(void) {
	struct {
		enum lockmsg msgcode;
		int chan;
	} msg;
	struct iovec iov[] = {
		{ &msg, sizeof(msg) },
	};

	int ret;
	struct iovec reply_iov[] = {
		{ &ret, sizeof(ret) },
	};

	int lock_fd = mkopenchan("/services/lock");

	struct pollresult pres;

	poll_add(lock_fd, POLL_RECV);

	while(1) {
		ASSERTNOERR(poll_wait(&pres));
		if(pres.chan == lock_fd) {
			/* Emulate a MsgReceive and MsgReply */
			int rcvchan = -1;
			int rcvlen = recv(lock_fd, iov, arraysize(iov), &rcvchan, RECV_NONBLOCK);
			if(rcvlen < 4) {
				printf("recv failed: %s (%d)", strerror(rcvlen), rcvlen);
				continue;
			}
			switch(msg.msgcode) {
			case LOCK_REGISTER_MSG:
				ret = poll_add(rcvchan, POLL_RECV);
				break;
			case LOCK_UNREGISTER_MSG:
				ret = poll_remove(msg.chan, POLL_ALL);
				break;
			default:
				ret = ENOFUNC;
			}
			if(rcvchan >= 0) {
				send(rcvchan, reply_iov, arraysize(reply_iov), -1, 0);
				if(msg.msgcode != LOCK_REGISTER_MSG)
					close(rcvchan);
			}
		} else if(pres.event == POLL_EVENT_RECV) {
			ret = recv(pres.chan, NULL, 0, NULL, RECV_NONBLOCK);
			if(ret < 0)
				continue;
			poll_remove(pres.chan, POLL_RECV);
			poll_add(pres.chan, POLL_SEND);
		} else if(pres.event == POLL_EVENT_SEND) {
			ret = send(pres.chan, NULL, 0, -1, SEND_NONBLOCK);
			if(ret < 0)
				continue;
			poll_remove(pres.chan, POLL_SEND);
			poll_add(pres.chan, POLL_RECV);
		} else {
			printf("Invalid poll response!\n");
		}
	}
}

void lockserver_init(void) {
	printf("lock init\n");

	xspawn(0, lockserver_task, SPAWN_DAEMON);
}
