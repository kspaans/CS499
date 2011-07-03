/* Standard library functions (stdlib/stdio) */
#ifndef LIB_H
#define LIB_H

#include <stdarg.h>
#include <types.h>
#include <syscall.h>

#define offsetof(st, m) __builtin_offsetof(st, m)
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

/* User mode asserts */
#define STRINGIFY(x) #x
#define STRINGIFY2(x) STRINGIFY(x)
#define ASSERT(x) do { \
	if(unlikely(!(x))) { \
		printf("\033[1;41mAssert \"" #x "\" failed at " __FILE__ ":" STRINGIFY2(__LINE__) ": "); \
		exit(); \
	} \
} while(0)
#define ASSERTNOERR(ret) do { \
	int rval = (ret); \
	if(unlikely(rval < 0)) { \
		printf("\033[1;41mAssert \"" #ret " >= 0\" failed at " __FILE__ ":" STRINGIFY2(__LINE__) ": "); \
		printf("Return value %d\033[m\n", rval); \
		exit(); \
	} \
} while(0)

/* Circular buffers */
#define QUEUE(T,Q) typedef struct { T *arr; int idx, len, max; } Q; \
	void Q##_init(Q *q, T *arr, int max); \
	void Q##_push(Q *q, T v); \
	T Q##_front(Q *q); \
	T Q##_pop(Q *q); \
	int Q##_empty(Q *q); \
	int Q##_full(Q *q);
QUEUE(char,charqueue)
QUEUE(int,intqueue)
#undef QUEUE

/* Generic printing functions */
// Function to send formatted strings to; returns >= 0 on success and <0 on failure
typedef int(*printfunc_t)(void *data, const char *str, size_t len);

int func_vprintf(printfunc_t printfunc, void *data, const char *fmt, va_list va);
int func_printf(printfunc_t printfunc, void *data, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

/* printf defined in server/console.c */
int vprintf(const char *fmt, va_list va);
int printf(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));

/* fprintf defined in server/console.c */
int vfprintf(int channel, const char *fmt, va_list va);
int fprintf(int channel, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

int vsprintf(char *buf, const char *fmt, va_list va);
int sprintf(char *buf, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

// *blocking* getchar.
int getchar(void);
int fgetc(int channel);
// putchar, equivalent to printf("%c", c);
void putchar(char c);
int fputc(char c, int channel);

/* stdlib functions */
int atoi(const char *str);
long strtol(const char *start, const char **end, int base);
unsigned long strtoul(const char *start, const char **end, int base);

/* Miscellaneous utilities */
// Destructively parse args from a buffer. argv will refer to positions within buf.
int parse_args(char *buf, char **argv, int argv_len);

size_t iov_length(const struct iovec *iov, int iovlen);
void iov_copy(const struct iovec *srciov, int srclen, const struct iovec *dstiov, int dstlen);

const char *strerror(int errno);
int xspawn(int priority, void (*code)(void), int flags);
ssize_t xsend(int chan, const struct iovec *iov, int iovlen, int sch, int flags);
ssize_t xrecv(int chan, const struct iovec *iov, int iovlen, int *rch, int flags);

#endif /* LIB_H */
