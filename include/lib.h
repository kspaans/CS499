/* Standard library functions (stdlib/stdio) */
#ifndef LIB_H
#define LIB_H

#include <stdarg.h>
#include <types.h>

/* User mode asserts */
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

/* Generic printing functions */
// Function to send formatted strings to
typedef void(*printfunc_t)(void *data, const char *str, size_t len);
int func_vprintf(printfunc_t printfunc, void *data, const char *fmt, va_list va);
int func_printf(printfunc_t printfunc, void *data, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

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

#endif /* LIB_H */
