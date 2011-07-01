/* Utility functions */

#include <lib.h>
#include <types.h>
#include <stdbool.h>
#include <hashtable.h>
#include <syscall.h>

/* Circular buffers */
#define QUEUE(T,Q) \
	void Q##_init(Q *q, T *arr, int max) \
		{ q->idx=q->len=0; q->max=max; q->arr=arr; } \
	void Q##_push(Q *q, T v) \
		{ q->arr[(q->idx+q->len)%q->max] = v; ++q->len; } \
	T Q##_front(Q *q) \
		{ return q->arr[q->idx]; } \
	T Q##_pop(Q *q) \
		{ T v = q->arr[q->idx]; q->idx = (q->idx+1)%q->max; --q->len; return v; } \
	int Q##_empty(Q *q) \
		{ return q->len == 0; } \
	int Q##_full(Q *q) \
		{ return q->len == q->max; }
QUEUE(char,charqueue)
QUEUE(int,intqueue)
#undef QUEUE

/* Hash tables */
static uint32_t default_hashfunc(const void *cmpkey) {
	return int_hash((uint32_t)cmpkey);
}
static int default_cmpfunc(const void *htkey, const void *cmpkey) {
	return (int)htkey-(int)cmpkey;
}
void hashtable_init(hashtable *ht, struct ht_item *arr, int max, ht_hashfunc_t hashfunc, ht_cmpfunc_t cmpfunc) {
	if(hashfunc == NULL) hashfunc = default_hashfunc;
	if(cmpfunc == NULL) cmpfunc = default_cmpfunc;
	ht->arr = arr;
	ht->count = 0;
	ht->max = max;
	ht->hashfunc = hashfunc;
	ht->cmpfunc = cmpfunc;
	for(int i=0; i<max; i++) {
		arr[i].valid = false;
	}
}

static inline int hashtable_get_entry(hashtable *ht, const void *cmpkey, bool accept_invalid) {
	uint32_t i = ht->hashfunc(cmpkey) % ht->max;
	uint32_t count = ht->max;
	while(ht->arr[i].valid) {
		if(!ht->arr[i].deleted && ht->cmpfunc(ht->arr[i].key, cmpkey) == 0) {
			return i;
		}
		if(accept_invalid && ht->arr[i].deleted)
			return i;
		++i;
		--count;
		if(i >= ht->max)
			i -= ht->max;
		if(count == 0)
			return HT_NOKEY;
	}
	if(accept_invalid)
		return i;
	return HT_NOKEY;
}

int hashtable_get(hashtable *ht, ht_key_t cmpkey) {
	return hashtable_get_entry(ht, cmpkey.voidval, false);
}

int hashtable_reserve(hashtable *ht, ht_key_t cmpkey) {
	return hashtable_get_entry(ht, cmpkey.voidval, true);
}

/* Argument parsing */
int parse_args(char *buf, char **argv, int argv_len) {
	int argc = 0;
	bool in_quote = false;
	char *cur = buf;
	char *argstart = buf;

	while(*buf) {
		char c = *buf++;

		if(in_quote) {
			if(c == '\\') {
				*cur++ = *buf++;
				continue;
			}
			if(c == '"')
				in_quote = 0;
			else
				*cur++ = c;
			continue;
		}

		switch(c) {
			case ' ':
			case '\t':
			case '\n':
				if(cur - argstart > 0 && argc < argv_len) {
					*cur++ = '\0';
					argv[argc++] = argstart;
					argstart = cur;
				}
				if(c == '\n') {
					goto end;
				}
				break;
			case '"':
				in_quote = 1;
				continue;
			case '\\':
				c = *buf++;
				if(c == '\n') {
					break;
				}
				// otherwise, fall through and add c
			default:
				*cur++ = c;
		}
	}
end:
	if(argc < argv_len) {
		argv[argc] = NULL;
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

int xspawn(int priority, void (*code)(void), int flags) {
	int tid = spawn(priority, code, flags);
	if (tid < 0) {
		printf("xspawn: failed to create task: %d", tid);
		exit();
	}
	return tid;
}

ssize_t xsend(int chan, const struct iovec *iov, int iovlen, int sch, int flags) {
	ssize_t ret = send(chan, iov, iovlen, sch, flags);
	if (ret < 0) {
		printf("xsend: failed to send: %d\n", ret);
		exit();
	}
	return ret;
}

ssize_t xrecv(int chan, const struct iovec *iov, int iovlen, int *rch, int flags) {
	ssize_t ret = recv(chan, iov, iovlen, rch, flags);
	if (ret < 0) {
		printf("xrecv: failed to recv: %d\n", ret);
		exit();
	}
	return ret;
}
