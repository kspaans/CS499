#ifndef HASHTABLE_H
#define HASHTABLE_H
/* Hash tables */
struct ht_item {
	uint32_t key;
	void *value;
	char deleted;
	char valid;
};

typedef uint32_t (*ht_hashfunc)(int key);
typedef int (*ht_cmpfunc)(int key1, int key2);

typedef struct {
	struct ht_item *arr;
	uint32_t count;
	uint32_t max;
	ht_hashfunc hashfunc;
	ht_cmpfunc cmpfunc;
} hashtable;

#define HT_NOKEY -1 // key not found
#define HT_NOMEM -2 // out of memory

static uint32_t default_hashfunc(int value) {
	return value;
}

static int default_cmpfunc(int key1, int key2) {
	return key1 - key2;
}

static void hashtable_init(hashtable *ht, struct ht_item *arr, int max, ht_hashfunc hashfunc, ht_cmpfunc cmpfunc) {
	if(hashfunc == NULL) hashfunc = default_hashfunc;
	if(cmpfunc == NULL) cmpfunc = default_cmpfunc;
	ht->arr = arr;
	ht->count = 0;
	ht->max = max;
	ht->hashfunc = hashfunc;
	ht->cmpfunc = cmpfunc;
	for(int i=0; i<max; i++) {
		arr[i].valid = 0;
	}
}

static int hashtable_get(hashtable *ht, int key, void **value) {
	uint32_t i = ht->hashfunc(key) % ht->max;
	uint32_t count = ht->max;
	while(ht->arr[i].valid) {
		if(ht->cmpfunc(key, ht->arr[i].key) == 0 && !ht->arr[i].deleted) {
			if(value) *value = ht->arr[i].value;
			return 0;
		}
		++i;
		--count;
		if(i >= ht->max)
			i -= ht->max;
		if(count == 0)
			return HT_NOKEY;
	}
	return HT_NOKEY;
}

static int hashtable_put(hashtable *ht, int key, void *value) {
	uint32_t i = ht->hashfunc(key) % ht->max;
	int count = ht->max;
	while(ht->arr[i].valid && !ht->arr[i].deleted
		&& ht->cmpfunc(key, ht->arr[i].key) != 0) {
		++i;
		--count;
		if(i >= ht->max)
			i -= ht->max;
		if(count == 0)
			return HT_NOMEM;
	}
	ht->arr[i].key = key;
	ht->arr[i].value = value;
	ht->arr[i].valid = 1;
	ht->arr[i].deleted = 0;
	return 0;
}

static int hashtable_del(hashtable *ht, int key) {
	uint32_t i = ht->hashfunc(key) % ht->max;
	int count = ht->max;
	while(ht->arr[i].valid) {
		if(ht->cmpfunc(key, ht->arr[i].key) == 0 && !ht->arr[i].deleted) {
			ht->arr[i].deleted = 1;
			return 0;
		}
		++i;
		--count;
		if(i >= ht->max)
			i -= ht->max;
		if(count == 0)
			return HT_NOKEY;
	}
	return HT_NOKEY;
}

#endif /* HASHTABLE_H */
