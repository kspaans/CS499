#ifndef INTTYPES_H
#define INTTYPES_H

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

#endif
