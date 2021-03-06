/* Utility functions */

#include <types.h>
#include <string.h>

const char *strchr(const char *s, int c) {
	while(*s) {
		if(*s == c)
			return s;
		s++;
	}
	return NULL;
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

int strncmp(const char *s1, const char *s2, size_t n) {
	char c1='\0', c2='\0';
	while(n > 0) {
		c1 = *s1++;
		c2 = *s2++;
		if(c1 == '\0' || c1 != c2)
			return c1-c2;
		n--;
	}
	return c1-c2;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	const char *a = s1, *b = s2;
	while(n-->0) {
		int ret = (*a++) - (*b++);
		if(ret)
			return ret;
	}
	return 0;
}

char *strcpy(char *d, const char *s) {
	size_t n = strlen(s);
	return memcpy(d, s, n+1);
}

size_t strlcpy(char *d, const char *s, size_t n) {
	size_t len = 0;
	while(*s && n > 1)
		*d++ = *s++, n--, len++;
	while(*s)
		s++, len++;
	if(n)
		*d = '\0';
	return len;
}

size_t strnlen(const char *s, size_t maxlen) {
	size_t len = 0;
	while(maxlen > 0 && *s++)
		len++, maxlen--;
	return len;
}
