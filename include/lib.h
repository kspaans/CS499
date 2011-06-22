/* Standard library functions (stdlib/stdio) */
#ifndef LIB_H
#define LIB_H

#include <stdarg.h>
#include <types.h>

#define offsetof(st, m) __builtin_offsetof(st, m)

/* User mode asserts */
#define STRINGIFY(x) #x
#define STRINGIFY2(x) STRINGIFY(x)
#define ASSERT(x) { \
	if(!(x)) { \
		printf("\033[1;41mAssert \"" #x "\" failed at " __FILE__ ":" STRINGIFY2(__LINE__) ": "); \
		exit(); \
	} \
}
#define ASSERTNOERR(ret) { \
	int rval = (ret); \
	if(rval < 0) { \
		printf("\033[1;41mAssert \"" #ret " >= 0\" failed at " __FILE__ ":" STRINGIFY2(__LINE__) ": "); \
		printf("Return value %d\033[m\n", rval); \
		exit(); \
	} \
}

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

/* Hash tables */
struct ht_item {
	uint32_t key;
	void *value;
	char deleted;
	char valid;
};

typedef uint32_t (*ht_hashfunc)(int key);
typedef int (*ht_cmpfunc)(int key1, int key2);

typedef struct {
	struct ht_item *arr;
	uint32_t count;
	uint32_t max;
	ht_hashfunc hashfunc;
	ht_cmpfunc cmpfunc;
} hashtable;

#define HT_NOKEY -1 // key not found
#define HT_NOMEM -2 // out of memory
void hashtable_init(hashtable *ht, struct ht_item *arr, int max, ht_hashfunc hashfunc, ht_cmpfunc cmpfunc);
int hashtable_get(hashtable *ht, int key, void **value);
int hashtable_put(hashtable *ht, int key, void *value);
int hashtable_del(hashtable *ht, int key);

/* Generic printing functions */
// Function to send formatted strings to
typedef void(*printfunc_t)(void *data, const char *str, size_t len);

int func_vprintf(printfunc_t printfunc, void *data, const char *fmt, va_list va);
int func_printf(printfunc_t printfunc, void *data, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

/* printf defined in server/console.c */
int vprintf(const char *fmt, va_list va);
int printf(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));

int vsprintf(char *buf, const char *fmt, va_list va);
int sprintf(char *buf, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

// *blocking* getchar.
int getchar();
// putchar, equivalent to printf("%c", c);
void putchar(char c);

/* stdlib functions */
int atoi(const char *str);
long strtol(const char *start, const char **end, int base);
unsigned long strtoul(const char *start, const char **end, int base);

/* Miscellaneous utilities */
// Destructively parse args from a buffer. argv will refer to positions within buf.
int parse_args(char *buf, char **argv, int argv_len);

int iov_length(const struct iovec *iov, int iovlen);
void iov_copy(const struct iovec *srciov, int srclen, const struct iovec *dstiov, int dstlen);

#endif /* LIB_H */
