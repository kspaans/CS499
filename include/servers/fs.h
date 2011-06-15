#ifndef SERVER_FS_H
#define SERVER_FS_H

#define PATH_MAX 1024
#define NAME_MAX 32

// fs interface
int open(int dirfd, const char *pathname);
int mkdir(int dirfd, const char *pathname);
int mkchan(int dirfd, const char *pathname);

// convenience functions
int sendpath(const char *pathname, int msgcode, const void *msg, int msglen, void *reply, int replylen, int *replychan);
int mkopenchan(const char *pathname);
void close(int dirfd);

// debug functions
void dump_files(void);

#endif /* SERVER_FS_H */
