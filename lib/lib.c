/* Utility functions */

#include <lib.h>
#include <types.h>

/* GCC hacks */
void raise() {}
// this is a silly hack to prevent a ton of GCC unwind garbage from being linked in
// (unwind functionality is loaded by long long division)
char __aeabi_unwind_cpp_pr0[0];

/* Circular buffers */
#define QUEUE(T,Q) \
	void Q##_init(Q *q, T *arr, int max) \
		{ q->idx=q->len=0; q->max=max; q->arr=arr; } \
	void Q##_push(Q *q, T v) \
		{ q->arr[(q->idx+q->len)%q->max] = v; ++q->len; } \
	T Q##_pop(Q *q) \
		{ T v = q->arr[q->idx]; q->idx = (q->idx+1)%q->max; --q->len; return v; } \
	int Q##_empty(Q *q) \
		{ return q->len == 0; } \
	int Q##_full(Q *q) \
		{ return q->len == q->max; }
QUEUE(char,charqueue)
QUEUE(int,intqueue)
#undef QUEUE

/* Argument parsing */
int parse_args(char *buf, char **argv, int argv_len) {
	int argc = 0;
	int in_arg = 0;
	while(*buf) {
		if(*buf == ' ') {
			*buf = '\0';
			in_arg = 0;
		} else if(!in_arg && argc < argv_len) {
			in_arg = 1;
			argv[argc] = buf;
			argc++;
		}
		buf++;
	}
	return argc;
}

/* Standard library stuff */
// _p_a2d: take a character and produce its digit value
static int _p_a2d(char c) {
	if(c>='0' && c<='9') return c-'0';
	else if(c>='a' && c<='z') return c-'a'+10;
	else if(c>='A' && c<='Z') return c-'A'+10;
	return -1;
}

unsigned long strtoul(const char *start, const char **end, int base) {
	unsigned long num=0;
	int digit;
	while((digit=_p_a2d(*start)) >= 0) {
		if(digit > base) break;
		num = num*base + digit;
		start++;
	}
	if(end) *end = start;
	return num;
}

long strtol(const char *start, const char **end, int base) {
	int sign = 0;
	if(*start == '-') {
		sign = 1;
		start++;
	} else if(*start == '+') {
		start++;
	}
	unsigned long num = strtoul(start, end, base);
	if(sign) return -num;
	return num;
}

int atoi(const char *s) {
	return strtol(s, NULL, 10);
}
