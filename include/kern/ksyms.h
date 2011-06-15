#ifndef KSYMS_H
#define KSYMS_H

#include <types.h>

struct ksym {
	uint32_t code;
	const char *name;
};

extern struct ksym ksyms_start;
extern struct ksym ksyms_end;

struct ksym *ksym_for_address(uint32_t code);
const char *symbol_for_address(uint32_t code);
const char *symbol_for_address_exact(uint32_t code);

#define SYMBOL(x) symbol_for_address((uint32_t)(x))
#define SYMBOL_EXACT(x) symbol_for_address_exact((uint32_t)(x))

#endif
