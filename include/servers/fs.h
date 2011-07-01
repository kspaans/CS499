#ifndef SERVER_FS_H
#define SERVER_FS_H

#define PATH_MAX 256
#define NAME_MAX 32
#define FILE_MAX 128

// fs interface
int open(int dirfd, const char *pathname);
int mkdir(int dirfd, const char *pathname);
int mkchan(int dirfd, const char *pathname);
int rmchan(int dirfd, const char *pathname);

// convenience functions
int xopen(int dirfd, const char *pathname);
int sendpath(const char *pathname, int msgcode, const void *msg, int msglen, void *reply, int replylen);
int mkopenchan(const char *pathname);

// debug functions
void dump_files(void);

void fileserver_init(void);

#endif /* SERVER_FS_H */
