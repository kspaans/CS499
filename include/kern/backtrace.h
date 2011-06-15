#ifndef BACKTRACE_H
#define BACKTRACE_H

#include <types.h>

void backtrace(void);
void unwind_stack(uint32_t *fp);

#endif
