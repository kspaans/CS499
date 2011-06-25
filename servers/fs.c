#include <lib.h>
#include <errno.h>
#include <event.h>
#include <syscall.h>
#include <task.h>
#include <string.h>
#include <servers/console.h>
#include <servers/fs.h>
#include <kern/printk.h>

enum filemsg {
	FILE_OPEN = PRIVATE_MSG_START,
	FILE_MKDIR,
	FILE_MKCHAN,
	FILE_RMCHAN,
};

static int file_req(int type, int dirfd, const char *pathname, bool expectfd) {
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
		close(fd);
	return ret;
}

int open(int dirfd, const char *pathname) {
	return file_req(FILE_OPEN, dirfd, pathname, true);
}

int xopen(int dirfd, const char *pathname) {
	int ret = file_req(FILE_OPEN, dirfd, pathname, true);
	if (ret < 0) {
		printf("xopen: failed to open %s (%d)\n", pathname, ret);
		exit();
	}
	return ret;
}

int mkdir(int dirfd, const char *pathname) {
	return file_req(FILE_MKDIR, dirfd, pathname, false);
}

int mkchan(int dirfd, const char *pathname) {
	return file_req(FILE_MKCHAN, dirfd, pathname, false);
}

int rmchan(int dirfd, const char *pathname) {
	return file_req(FILE_RMCHAN, dirfd, pathname, false);
}

int mkopenchan(const char *pathname) {
	int ret = mkchan(ROOT_DIRFD, pathname);
	if (ret) {
		printf("mkopenchan: failed to make %s (%d)\n", pathname, ret);
		exit();
	}
	int fd = open(ROOT_DIRFD, pathname);
	if (fd < 0) {
		printf("mkopenchan: failed to open %s (%d)\n", pathname, fd);
		exit();
	}
	return fd;
}

int sendpath(const char *pathname, int msgcode, const void *msg, int msglen, void *reply, int replylen) {
	int fd = open(ROOT_DIRFD, pathname);
	if (fd < 0) {
		printf("sendpath: failed to open %s (%d)\n", pathname, fd);
		exit();
	}
	int ret = MsgSend(fd, msgcode, msg, msglen, reply, replylen, NULL);
	close(fd);
	return ret;
}

#define HASH_MAX FILE_MAX
/* eventually, we may want to set HASH_MAX > FILE_MAX to keep the load factor low */

struct fastfs {
	struct {
		char path[PATH_MAX];
		size_t pathlen;
		int chan;
		bool valid, deleted;
	} files[HASH_MAX];
	size_t count;
} fs;

/* djb2 */
static uint32_t fs_hash(const char *path, size_t pathlen) {
	uint32_t hash = 5381;

	while(pathlen-- > 0)
		hash = ((hash << 5) + hash) ^ (*path++); // hash * 33 + c
	return hash;
}

static inline int hash_get_entry(const char *path, size_t pathlen, bool accept_invalid) {
	int i = fs_hash(path, pathlen) % HASH_MAX;
	int count = HASH_MAX;
	while(fs.files[i].valid) {
		if(!fs.files[i].deleted
		  && fs.files[i].pathlen == pathlen
		  && memcmp(fs.files[i].path, path, pathlen) == 0) {
			return i;
		}
		if(accept_invalid && fs.files[i].deleted)
			return i;
		++i;
		--count;
		if(i >= HASH_MAX)
			i -= HASH_MAX;
		if(count == 0)
			return HT_NOKEY;
	}
	if(accept_invalid)
		return i;
	else
		return HT_NOKEY;
}

static int fs_get(const char *path, size_t pathlen) {
	return hash_get_entry(path, pathlen, false);
}

static int fs_reserve(const char *path, size_t pathlen) {
	return hash_get_entry(path, pathlen, true);
}

static int fs_delete(const char *path, size_t pathlen) {
	int i = hash_get_entry(path, pathlen, false);
	if(i >= 0)
		fs.files[i].deleted = 1;
	return i;
}

static void do_open(int dirfd, int tid, const char *path, size_t pathlen) {
	int i = fs_get(path, pathlen);
	if(i < 0) {
		MsgReplyStatus(tid, ENOENT);
		return;
	}

	MsgReply(tid, 0, NULL, 0, fs.files[i].chan);
}

static void do_mkdir(int dirfd, int tid, const char *path, size_t pathlen) {
	/* TODO */
	MsgReplyStatus(tid, ENOFUNC);
}

static void do_mkchan(int dirfd, int tid, const char *path, size_t pathlen) {
	int i = fs_reserve(path, pathlen);
	if(i < 0) {
		MsgReplyStatus(tid, ENOSPC);
		return;
	}
	if(fs.files[i].valid && !fs.files[i].deleted) {
		MsgReplyStatus(tid, EEXIST);
		return;
	}
	int chan = channel(0);
	if(chan < 0) {
		MsgReplyStatus(tid, ENOSPC);
		return;
	}

	fs.files[i].valid = true;
	fs.files[i].deleted = false;
	fs.files[i].chan = chan;
	fs.files[i].pathlen = pathlen;
	memcpy(fs.files[i].path, path, pathlen);
	MsgReplyStatus(tid, 0);
}

static void do_rmchan(int dirfd, int tid, const char *path, size_t pathlen) {
	/* TODO: security */
	int i = fs_delete(path, pathlen);
	if(i < 0) {
		MsgReplyStatus(tid, ENOENT);
		return;
	}
	close(fs.files[i].chan);
	MsgReplyStatus(tid, 0);
}

static void add_chan(int dirfd, const char *path, int chan) {
	size_t pathlen = strlen(path);
	int i = fs_reserve(path, pathlen);
	if(i < 0)
		return;
	fs.files[i].valid = true;
	fs.files[i].deleted = false;
	fs.files[i].chan = chan;
	fs.files[i].pathlen = pathlen;
	memcpy(fs.files[i].path, path, pathlen);
}

void fileserver_task(void) {
	int tid, msgcode;
	char path[PATH_MAX];

	printf("fileserver init\n");

	for(int i=0; i<HASH_MAX; ++i) {
		fs.files[i].pathlen = 0;
		fs.files[i].valid = false;
	}

	add_chan(ROOT_DIRFD, "/dev/conin", STDIN_FILENO);
	add_chan(ROOT_DIRFD, "/dev/conout", STDOUT_FILENO);
	add_chan(ROOT_DIRFD, "/", ROOT_DIRFD);

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
			case FILE_RMCHAN:
				do_rmchan(ROOT_DIRFD, tid, path, len);
				break;
			default:
				MsgReplyStatus(tid, EINVAL);
				break;
		}
	}
}

void dump_files(void) {
	printk("filesystem dump:\n");
	for (int i = 0; i < HASH_MAX; ++i) {
		if (fs.files[i].valid && !fs.files[i].deleted)
			printk("[%d] %.*s\n", i, PATH_MAX, fs.files[i].path);
	}
}
