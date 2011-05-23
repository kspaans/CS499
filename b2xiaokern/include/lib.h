/* Standard library functions */
#ifndef KERNLIB_H
#define KERNLIB_H

#include <stdarg.h>

/* Generally useful constants */
#define NULL ((void*)0)
#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0

/* *User* mode asserts */
#define STRINGIFY(x) #x
#define STRINGIFY2(x) STRINGIFY(x)
#define ASSERT(str, a, b) { \
	if((a) != (b)) { \
		printk("\033[1;41mAssert \"" #a " == " #b "\" failed at " __FILE__ ":" STRINGIFY2(__LINE__) ": "); \
		printk("%s: ", str); \
		printk( #a "=%d, " #b "=%d\033[m\n", (int)(a), (int)(b)); \
		Exit(); \
	} \
}
#define ASSERTNOERR(ret) { \
	int rval = (ret); \
	if(rval < 0) { \
		printk("\033[1;41mAssert \"" #ret " >= 0\" failed at " __FILE__ ":" STRINGIFY2(__LINE__) ": "); \
		printk("Return value %d\033[m\n", rval); \
		Exit(); \
	} \
}

/* Circular buffers */
typedef struct {
	char *arr;
	int idx, len, max;
} queue;

typedef struct {
	int *arr;
	int idx, len, max;
} intqueue;

void queue_pushback(queue *buf, char byte);
void queue_pushcircular(queue *buf, char byte);
char queue_popfront(queue *buf);
void queue_pushfront(queue *buf, char c);
void queue_clear(queue *buf);

void intqueue_pushback(intqueue *buf, int byte);
int intqueue_popfront(intqueue *buf);
void intqueue_clear(intqueue *buf);

/* Generic printing functions */
// Function to send formatted strings to
typedef void(*printfunc_t)(void *data, const char *str, unsigned long len);
int func_vprintf(printfunc_t printfunc, void *data, const char *fmt, va_list va);
int func_printf(printfunc_t printfunc, void *data, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

int vsprintf(char *buf, const char *fmt, va_list va);
int sprintf(char *buf, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

/* Kernel printf */
int vprintk(const char *fmt, va_list va);
int printk(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));
void kputs(const char *str);
// *blocking* getchar.
int getchar();
// putchar, equivalent to printf("%c", c);
void putchar(char c);

/* stdlib functions */
int atoi(const char *str);
long strtol(const char *start, const char **end, int base);
unsigned long strtoul(const char *start, const char **end, int base);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int len);
int strlen(const char* s);
int memcmp(const void *s1, const void *s2, int len);
void *memset(void *p, int b, int size);

/* Miscellaneous utilities */
// Destructively parse args from a buffer. argv will refer to positions within buf.
int parse_args(char *buf, char **argv, int argv_len);
void memcpy(void *dest, const void *src, int len);
void copy_aligned_region(int *dest, const int *src, int len);

#endif /* KERNLIB_H */
