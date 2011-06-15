#ifndef SERVER_FS_H
#define SERVER_FS_H

#define PATH_MAX 1024
#define NAME_MAX 32

int open(int dirfd, const char *pathname);
void close(int dirfd);
int mkdir(int dirfd, const char *pathname);
int mkchan(int dirfd, const char *pathname);

int sendpath(const char *pathname, int msgcode, const void *msg, int msglen, void *reply, int replylen, int *replychan);
int mkopenchan(const char *pathname);

#endif /* SERVER_FS_H */
