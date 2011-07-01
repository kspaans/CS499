#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <lib.h>
#include <types.h>
#include <stdbool.h>

#define HT_NOKEY (-1) /* key not found */
#define HT_NOMEM (-2) /* out of memory */

typedef union {
	const void *voidval;
	intptr_t intval;
	uintptr_t uintval;
} ht_key_t __attribute__((__transparent_union__));

struct ht_item {
	union {
		void *key;
		intptr_t intkey;
	};
	union {
		void *value;
		intptr_t intvalue;
	};
	bool deleted;
	bool valid;
};

typedef uint32_t (*ht_hashfunc_t)(const void *cmpkey);
typedef int (*ht_cmpfunc_t)(const void *htkey, const void *cmpkey);

typedef struct {
	struct ht_item *arr;
	size_t count;
	size_t max;
	ht_hashfunc_t hashfunc;
	ht_cmpfunc_t cmpfunc;
} hashtable;

void hashtable_init(hashtable *ht, struct ht_item *arr, int max, ht_hashfunc_t hashfunc, ht_cmpfunc_t cmpfunc);
/* Lookup an element and return its index in the item array; return <0 if not found */
int hashtable_get(hashtable *ht, ht_key_t cmpkey);
/* Lookup an element and return its index in the item array if found, else return the first inactive item */
int hashtable_reserve(hashtable *ht, ht_key_t cmpkey);

static inline bool active_ht_item(struct ht_item *item) {
	return (item->valid && !item->deleted);
}
static inline void delete_ht_item(struct ht_item *item) {
	ASSERT(active_ht_item(item));
	item->deleted = true;
}
static inline void activate_ht_item(struct ht_item *item) {
	ASSERT(!active_ht_item(item));
	item->valid = true;
	item->deleted = false;
}

/* http://www.concentric.net/~ttwang/tech/inthash.htm */
/* This is very fast on ARM, taking only 9 instructions
   by using shifted operands on arithmetic operations */
static inline uint32_t int_hash(uint32_t key) {
	key = ~key + (key << 15); // key = (key << 15) - key - 1
	key = key ^ (key >> 12);
	key = key + (key << 2);
	key = key ^ (key >> 4);
	key = key + (key << 3) + (key << 11);
	key = key ^ (key >> 16);
	return key;
}
/* djb2: http://www.cse.yorku.ca/~oz/hash.html */
static inline uint32_t str_hash(const char *str, size_t len) {
	uint32_t hash = 5381;

	while(len-- > 0)
		hash = ((hash << 5) + hash) ^ (*str++); // hash * 33 ^ c
	return hash;
}

#endif /* HASHTABLE_H */
