#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define arraysize(x) (sizeof(x)/sizeof(*(x)))

typedef signed   int8_t   __attribute__((__mode__(__QI__)));
typedef unsigned uint8_t  __attribute__((__mode__(__QI__)));

typedef signed   int16_t  __attribute__((__mode__(__HI__)));
typedef unsigned uint16_t __attribute__((__mode__(__HI__)));

typedef signed   int32_t  __attribute__((__mode__(__SI__)));
typedef unsigned uint32_t __attribute__((__mode__(__SI__)));

typedef signed   int64_t  __attribute__((__mode__(__DI__)));
typedef unsigned uint64_t __attribute__((__mode__(__DI__)));

typedef signed   intptr_t  __attribute__((__mode__(word)));
typedef unsigned uintptr_t __attribute__((__mode__(word)));

typedef signed   ssize_t   __attribute__((__mode__(word)));
typedef unsigned size_t    __attribute__((__mode__(word)));

#define NULL ((void *)0)

typedef uint32_t in_addr_t;

struct in_addr {
	in_addr_t s_addr;
};

typedef struct mac_addr {
	uint8_t addr[6];
} mac_addr_t;

struct hostdata {
	const char *hostname;
	struct mac_addr mac;
	uint32_t ip;
	uint32_t ncip;
	uint16_t ncport;
	bool has_uart;
};

#endif /* TYPES_H */
