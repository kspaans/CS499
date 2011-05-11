#include <inttypes.h>
#include <string.h>

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s2 && *s1 == *s2)
    s1++, s2++;
  return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if (!n)
    return 0;
  while (*s1 && *s2 && *s1 == *s2 && --n)
    s1++, s2++;
  return *s1 - *s2;
}

size_t strlcpy(char *destination, const char *source, size_t size) {
  size_t len = 0;
  while (*source && size > 1)
    *destination++ = *source++, size--, len++;
  while (*source)
    source++, len++;
  if (size)
    *destination = '\0';
  return len;
}

size_t strlen(const char *s) {
  size_t len = 0;
  while (*s)
    s++, len++;
  return len;
}

size_t strnlen(const char *s, size_t maxlen) {
  size_t len = 0;
  while (maxlen > 0 && *s)
    s++, len++, maxlen--;
  return len;
}

void *memset(void *p, int b, size_t size) {
  char *c = p;
  for (size_t i = 0; i < size; ++i)
    c[i] = b;
  return c;
}

void memzero(void *p, size_t size) {
  char *dst = p;

  while (size >= 8) {
    *dst++ = 0;
    *dst++ = 0;
    *dst++ = 0;
    *dst++ = 0;
    *dst++ = 0;
    *dst++ = 0;
    *dst++ = 0;
    *dst++ = 0;
    size -= 8;
  }

  if (size & 4) {
    *dst++ = 0;
    *dst++ = 0;
    *dst++ = 0;
    *dst++ = 0;
  }

  if (size & 2) {
    *dst++ = 0;
    *dst++ = 0;
  }

  if (size & 1)
    *dst++ = 0;
}

void *memcpy(void *d, const void *s, size_t size) {
  char *dst = d;
  const char *src = s;

  while (size >= 8) {
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    size -= 8;
  }

  if (size & 4) {
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
  }

  if (size & 2) {
    *dst++ = *src++;
    *dst++ = *src++;
  }

  if (size & 1)
    *dst++ = *src++;

  return d;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *a = s1, *b = s2;

  for (size_t i = 0; i < n; ++i) {
    int ret = a[i] - b[i];
    if (ret)
      return ret;
  }

  return 0;
}

char *strcpy(char *dest, const char *src) {
  char *p = dest;
  while (*src)
    *(p++) = *(src++);
  *p = '\0';
  return dest;
}

const char *strchr(const char *s, int c) {
  for (size_t i = 0; i < strlen(s); ++i)
    if (s[i] == c)
      return &s[i];
  return NULL;
}
