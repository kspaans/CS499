#include <lib.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <task.h>
#include <string.h>
#include <servers/fs.h>
#include <kern/printk.h>

#define FILE_MAX 32

enum filemsg {
	FILE_OPEN,
	FILE_MKDIR,
	FILE_MKCHAN,
};

static int file_req(int type, int dirfd, const char *pathname, int expectfd) {
	int ret, reply, fd = -1;

	reply = MsgSend(dirfd, type, pathname, strlen(pathname), NULL, 0,
		        expectfd ? &fd : NULL);
	if (reply < 0) {
		ret = reply;
		goto fail;
	}
	if (expectfd && fd < 0) {
		ret = EBADMSG;
		goto fail;
	}

	return expectfd ? fd : 0;
fail:
	if (fd != -1)
		ChannelClose(fd);
	return ret;
}

int open(int dirfd, const char *pathname) {
	return file_req(FILE_OPEN, dirfd, pathname, 1);
}

void close(int dirfd) {
	ChannelClose(dirfd);
}

int mkdir(int dirfd, const char *pathname) {
	return file_req(FILE_MKDIR, dirfd, pathname, 0);
}

int mkchan(int dirfd, const char *pathname) {
	return file_req(FILE_MKCHAN, dirfd, pathname, 0);
}

int mkopenchan(const char *pathname) {
	int ret = mkchan(ROOT_DIRFD, pathname);
	if (ret) {
		printf("mkopenchan: failed to make %s (%d)\n", pathname, ret);
		return ret;
	}
	int fd = open(ROOT_DIRFD, pathname);
	if (fd < 0) {
		printf("mkopenchan: failed to open %s (%d)\n", pathname, fd);
		return fd;
	}
	return fd;

}

int sendpath(const char *pathname, int msgcode, const void *msg, int msglen, void *reply, int replylen) {
	int fd = open(ROOT_DIRFD, pathname);
	if (fd < 0) {
		printf("sendpath: failed to open %s (%d)\n", pathname, fd);
		Exit();
	}
	int ret = MsgSend(fd, msgcode, msg, msglen, reply, replylen, NULL);
	close(fd);
	return ret;
}

struct fastfs {
	struct {
		char path[PATH_MAX];
		int pathlen;
		int chan;
	} files[FILE_MAX];
} fs;

static void do_open(int dirfd, int tid, const char *path, size_t pathlen) {
	for (int i = 0; i < FILE_MAX; ++i) {
		if (fs.files[i].pathlen != pathlen)
			continue;
		if (memcmp(fs.files[i].path, path, pathlen))
			continue;
		MsgReply(tid, 0, NULL, 0, fs.files[i].chan);
		return;
	}

	MsgReplyStatus(tid, ENOENT);
}

static void do_mkdir(int dirfd, int tid, const char *path, size_t pathlen) {
	MsgReplyStatus(tid, 0);
}

static void do_mkchan(int dirfd, int tid, const char *path, size_t pathlen) {
	for (int i = 0; i < FILE_MAX; ++i) {
		if (!fs.files[i].pathlen) {
			fs.files[i].chan = ChannelOpen();
			fs.files[i].pathlen = pathlen;
			memcpy(fs.files[i].path, path, pathlen);
			MsgReplyStatus(tid, 0);
			return;
		}
	}

	MsgReplyStatus(tid, ENOSPC);
}

void fileserver_task(void) {
	int tid, msgcode;
	char path[PATH_MAX];

	printf("fileserver init\n");

	for (;;) {
		int len = MsgReceive(ROOT_DIRFD, &tid, &msgcode, path, sizeof(path));
		if (len < 0) {
			printf("fileserver failed receive (%d)", len);
			continue;
		}
		switch (msgcode) {
			case FILE_OPEN:
				do_open(ROOT_DIRFD, tid, path, len);
				break;
			case FILE_MKDIR:
				do_mkdir(ROOT_DIRFD, tid, path, len);
				break;
			case FILE_MKCHAN:
				do_mkchan(ROOT_DIRFD, tid, path, len);
				break;
			default:
				MsgReplyStatus(tid, EINVAL);
				break;
		}

	}
}

void dump_files(void) {
	char path[PATH_MAX];

	printk("filesystem dump:\n");
	for (int i = 0; i < FILE_MAX; ++i) {
		if (fs.files[i].pathlen) {
			memcpy(path, fs.files[i].path, PATH_MAX);
			path[PATH_MAX-1] = '\0';
			printk("[%d] %s\n", i, path);
		}
	}
}
