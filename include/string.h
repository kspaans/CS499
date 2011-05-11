#ifndef STRING_H
#define STRING_H

#include <inttypes.h>

#define NULL ((void *)0)

#define memcpy(dst, src, n) __builtin_memcpy(dst, src, n)
#define memcmp(s1, s2, n) __builtin_memcmp(s1, s2, n)
#define memchr(s, c, n) __builtin_memchr(s, c, n)
#define memset(dst, ch, size) __builtin_memset(dst, ch, size)

#define strchr(s, c) __builtin_strchr(s, c)
#define strcmp(s1, s2) __builtin_strcmp(s1, s2)
#define strncmp(s1, s2, n) __builtin_strncmp(s1, s2, n)
#define strlen(s) __builtin_strlen(s)
#define strnlen(s, n) __builtin_strnlen(s, n)
#define strcat(dst, src) __builtin_strcat(dst, src)
#define strncat(dst, src, n) __builtin_strcat(dst, src, n)
#define strcpy(dst, src) __builtin_strcpy(dst, src)
#define strncpy(dst, src, n) __builtin_strncpy(dst, src, n)

const char *strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
int strcasecmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strnlen(const char *s, size_t maxlen);
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
void *memcpy(void * d, const void * s, size_t size);
int memcmp(const void *s1, const void *s2, size_t n);

void memzero(void *p, size_t size);
void *memset(void *p, int b, size_t size);
size_t strlcpy(char *destination, const char *source, size_t size);

#endif
