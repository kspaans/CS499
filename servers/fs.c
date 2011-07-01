#include <lib.h>
#include <errno.h>
#include <event.h>
#include <hashtable.h>
#include <syscall.h>
#include <task.h>
#include <string.h>
#include <servers/console.h>
#include <servers/fs.h>

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

/* Use 2*FILE_MAX to minimize collisions */
#define HASH_MAX (2*FILE_MAX)

struct fs_key {
	char path[PATH_MAX];
	size_t pathlen;
};

struct fastfs {
	struct ht_item ht_arr[HASH_MAX];
	struct fs_key ht_keys[HASH_MAX];
	hashtable ht; // fs_key -> int (channel number)
} rootfs;

static uint32_t fs_hashfunc(const struct fs_key *cmpkey) {
	return str_hash(cmpkey->path, cmpkey->pathlen);
}

static int fs_cmpfunc(const struct fs_key *htkey, const struct fs_key *cmpkey) {
	int diff = htkey->pathlen - cmpkey->pathlen;
	if(diff)
		return diff;
	return memcmp(htkey->path, cmpkey->path, htkey->pathlen);
}

static void fs_init(struct fastfs *fs) {
	hashtable_init(&fs->ht, fs->ht_arr, HASH_MAX, (ht_hashfunc_t)fs_hashfunc, (ht_cmpfunc_t)fs_cmpfunc);
}

static int do_open(struct fastfs *fs, const struct fs_key *key) {
	int i = hashtable_get(&fs->ht, key);
	if(i < 0)
		return ENOENT;

	return i;
}

static int do_mkdir(struct fastfs *fs, const struct fs_key *key) {
	/* TODO */
	return ENOFUNC;
}

static int do_mkchan(struct fastfs *fs, const struct fs_key *key, int chan) {
	int i = hashtable_reserve(&fs->ht, key);
	if(i < 0)
		return ENOSPC;

	if(active_ht_item(&fs->ht_arr[i]))
		return EEXIST;

	if(chan < 0)
		chan = channel(0);
	if(chan < 0)
		return ENOSPC;

	fs->ht_keys[i].pathlen = key->pathlen;
	memcpy(fs->ht_keys[i].path, key->path, key->pathlen);

	fs->ht_arr[i].key = &fs->ht_keys[i];
	fs->ht_arr[i].intvalue = chan;
	activate_ht_item(&fs->ht_arr[i]);
	return i;
}

static int do_rmchan(struct fastfs *fs, const struct fs_key *key) {
	/* TODO: security */
	int i = hashtable_get(&fs->ht, key);
	if(i < 0)
		return ENOENT;

	close(fs->ht_arr[i].intvalue);
	delete_ht_item(&fs->ht_arr[i]);
	return 0;
}

static int add_chan(struct fastfs *fs, const char *path, int chan) {
	struct fs_key key;
	key.pathlen = strlen(path);
	memcpy(key.path, path, key.pathlen);

	return do_mkchan(fs, &key, chan);
}

static void fileserver_task(void) {
	int tid, msgcode;
	struct fs_key key;

	printf("fileserver init\n");

	fs_init(&rootfs);

	add_chan(&rootfs, "/dev/conin", STDIN_FILENO);
	add_chan(&rootfs, "/dev/conout", STDOUT_FILENO);
	add_chan(&rootfs, "/", ROOT_DIRFD);

	for (;;) {
		int len = MsgReceive(ROOT_DIRFD, &tid, &msgcode, key.path, sizeof(key.path));
		if (len < 0) {
			printf("fileserver failed receive (%d)", len);
			continue;
		}
		key.pathlen = len;
		int ret;
		switch (msgcode) {
			case FILE_OPEN:
				ret = do_open(&rootfs, &key);
				if(ret >= 0) {
					MsgReply(tid, 0, NULL, 0, rootfs.ht_arr[ret].intvalue);
					continue;
				}
				break;
			case FILE_MKDIR:
				ret = do_mkdir(&rootfs, &key);
				break;
			case FILE_MKCHAN:
				ret = do_mkchan(&rootfs, &key, -1);
				break;
			case FILE_RMCHAN:
				ret = do_rmchan(&rootfs, &key);
				break;
			default:
				ret = EINVAL;
				break;
		}
		if(ret > 0)
			ret = 0;
		MsgReplyStatus(tid, ret);
	}
}

void dump_files(void) {
	for (int i = 0; i < HASH_MAX; ++i) {
		if (active_ht_item(&rootfs.ht_arr[i]))
			printf("[%d] %.*s\n", i, rootfs.ht_keys[i].pathlen, rootfs.ht_keys[i].path);
	}
}

void fileserver_init(void) {
	xspawn(2, fileserver_task, SPAWN_DAEMON);
}
