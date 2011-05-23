/* Utility functions */

#include "lib.h"

/* GCC hacks */
void raise() {}
// this is a silly hack to prevent a ton of GCC unwind garbage from being linked in
// (unwind functionality is loaded by long long division)
char __aeabi_unwind_cpp_pr0[0];

/* Queue functions */
void queue_pushcircular(queue *buf, char c) {
	int i = (buf->idx + buf->len)%buf->max;
	buf->arr[i] = c;
	if(buf->len < buf->max)
		buf->len++;
	else
		buf->idx = (buf->idx+1)%buf->max;
}

void queue_pushback(queue *buf, char c) {
	if(buf->len < buf->max) {
		queue_pushcircular(buf, c);
	}
}

void queue_pushfront(queue *buf, char c) {
	buf->len++;
	buf->idx = (buf->idx - 1)%buf->max;
	buf->arr[buf->idx] = c;
}

char queue_popfront(queue *buf) {
	char c = buf->arr[buf->idx];
	buf->idx = (buf->idx + 1)%buf->max;
	buf->len--;
	return c;
}

void queue_clear(queue *buf) {
	buf->len = 0;
}

void intqueue_pushback(intqueue *buf, int c) {
	if(buf->len < buf->max) {
		int i = (buf->idx + buf->len)%buf->max;
		buf->arr[i] = c;
		buf->len++;
	}
}

int intqueue_popfront(intqueue *buf) {
	int c = buf->arr[buf->idx];
	buf->idx = (buf->idx + 1)%buf->max;
	buf->len--;
	return c;
}

void intqueue_clear(intqueue *buf) {
	buf->len = 0;
}

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

int strcmp(const char *s1, const char *s2) {
	char c1, c2;
	do {
		c1 = *s1++;
		c2 = *s2++;
		if(c1 == '\0')
			break;
	} while(c1 == c2);
	return c1-c2;
}

int strncmp(const char *s1, const char *s2, int len) {
	char c1='\0', c2='\0';
	while(len > 0) {
		c1 = *s1++;
		c2 = *s2++;
		if(c1 == '\0' || c1 != c2)
			return c1-c2;
		len--;
	}
	return c1-c2;
}

int strlen(const char* s) {
	int len = 0;
	while(*s++) {
		len++;
	}
	return len;
}


int memcmp(const void *s1, const void *s2, int len) {
	const char *a = s1, *b = s2;

	while(len-->0) {
		int ret = (*a++) - (*b++);
		if(ret)
			return ret;
	}

	return 0;
}

void *memset(void *p, int b, int size) {
	char *c = p;
	for(int i = 0; i < size; ++i)
		c[i] = b;
	return c;
}
