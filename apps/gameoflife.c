/*
 * Kyle's simple application: Game of Life
 * use the standard 3/23 rule
 */
#include <types.h>
#include <mem.h>
#include <ip.h>
#include <servers/net.h>
#include <string.h>
#include <lib.h>
#include <apps.h>
#include <hashtable.h>
#include "gameoflife.h"

#define X_SIZE 100
#define Y_SIZE 100
#define HASH_SIZE 303
#define MTU 1500

#define UDP_SRCPORT 8889
#define UDP_DSTPORT UDP_SRCPORT
#define GUMSTIX_CS IP(10, 0, 0, 1)

int livecnt = 0;

/*
 * Simple hash for the Game Of Life that puts all the weight on the low 16 bits
 * of the coordinates, effectively assigning unique hashes within a 64k x 64k
 * field.
 */
static uint32_t life_hashfunc(uint32_t x, uint32_t y)
{
	x &= 0x0000FFFF;
	x <<= 16;
	y &= 0x0000FFFF;
	return x | y;
}

/* This structure abstracts the implementation of the field.
   You can replace the implementations of the init, get, add, del
   functions as you see fit. */
struct field {
	struct ht_item ht_arr[HASH_SIZE];
	hashtable ht;
	/* add more fields as appropriate... */
};

static void field_init(struct field *field) {
	hashtable_init(&field->ht, field->ht_arr, HASH_SIZE, NULL, NULL);
}

/* Returns 1 if the cell is alive and 0 otherwise */
static int field_get(struct field *field, size_t x, size_t y) {
	return (hashtable_get(&field->ht, life_hashfunc(x, y)) != HT_NOKEY);
}

static void field_add(struct field *field, size_t x, size_t y) {
	uint32_t key = life_hashfunc(x, y);
	/* Robert sez:
	   New hashtable implementation is a bit different.
	   Instead of doing a "put", you "reserve" a spot in the hashtable.
	   This spot might be occupied by your key already, in which case
	   you should update the entry as appropriate.
	   Note, however, that the spot is not truly yours until you activate
	   it. Do not call reserve twice in a row without activating an entry first
	   or the first reservation may not be valid. */
	int i = hashtable_reserve(&field->ht, key);
	if(i < 0) {
		/* Ran out of space...this is fatal */
		printf("oh god, we have too many live cells\n");
		return;
	}
	if(active_ht_item(&field->ht_arr[i])) {
		/* Entry is already active. */
		return;
	}
	field->ht_arr[i].intkey = key;
	field->ht_arr[i].intvalue = 1;
	activate_ht_item(&field->ht_arr[i]);
	++livecnt;
}

static void field_del(struct field *field, size_t x, size_t y) {
	int i = hashtable_get(&field->ht, life_hashfunc(x, y));
	if(i < 0) {
		/* Entry was not active to begin with */
		return;
	}
	delete_ht_item(&field->ht_arr[i]);
	--livecnt;
}

/*
 * Put (0,0) cell in the upper-left
 */
static void display(struct field *field, size_t x, size_t y)
{
	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			printf("%d ", field_get(field, i, j));
		}
		printf("\n");
	}
}

/*
 * To be more frugal, send only the live cells to the other end.
 */
static void display_json(struct field *field, size_t x, size_t y)
{
	char buf[MTU] = {};
	size_t idx = 0;
	uint8_t flag_firstloop = 1;

	idx += sprintf(buf, "{");
	idx += sprintf(buf + idx, "\"x\":%d,\"y\":%d,\"f\":[", X_SIZE, Y_SIZE);
	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			if (field_get(field, i, j)) {
				if (!flag_firstloop) {
					idx += sprintf(buf + idx, ",");
					if (idx >= MTU) goto error_send;
				} else {
					flag_firstloop = 0;
				}
				idx += sprintf(buf + idx, "[%d,%d]", i, j);
				if (idx >= MTU) goto error_send;
			}
		}
	}
	idx += sprintf(buf + idx, "]}");
	if (idx >= MTU) goto error_send;
	send_udp(UDP_SRCPORT, GUMSTIX_CS, UDP_DSTPORT, buf, idx);
	return;

error_send:
	printf("ERROR: Whoops, overfilled the UDP buffer: %d\n", idx);
	send_udp(UDP_SRCPORT, GUMSTIX_CS, UDP_DSTPORT, buf, MTU);
}

/*
 * Use wrap-around semantics for the edges of the field
 */
static uint8_t surround(struct field *field, size_t curx, size_t cury, size_t x, size_t y)
{
	uint8_t count = 0;
	int8_t  leftmost = curx - 1;
	int8_t  topmost = cury - 1;

	if (leftmost < 0) {
		leftmost = x - 1;
	}
	if (topmost < 0) {
		topmost = y - 1;
	}

	count += field_get(field, leftmost, topmost);
	count += field_get(field, leftmost, (topmost + 1) % y);
	count += field_get(field, leftmost, (topmost + 2) % y);
	count += field_get(field, (leftmost + 1) % x, topmost);
	count += field_get(field, (leftmost + 1) % x, (topmost + 2) % y);
	count += field_get(field, (leftmost + 2) % x, topmost);
	count += field_get(field, (leftmost + 2) % x, (topmost + 1) % y);
	count += field_get(field, (leftmost + 2) % x, (topmost + 2) % y);
	return count;
}

/*
 * A little bit gross, we need access to the hash table store so that it can be
 * copied -- this gives us an unchanging copy of the field with which to use
 * while computing the next generation.
 * A possible extention would be a hast_table_foreach();
 */
static void age(struct field *field, size_t x, size_t y)
{
	struct field copy;
	memcpy(&copy, field, sizeof(copy));
	copy.ht.arr = copy.ht_arr;

	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			uint8_t count = surround(&copy, i, j, x, y);
			if (count > 3 || count < 2) {
				field_del(field, i, j); // cell dies
			} else if (count == 3) {
				field_add(field, i, j); // cell is born
			}
		}
	}
}

void gameoflife(void)
{
	struct field field;

	field_init(&field);
	/*
	// start it with a glider
	field_add(&field, 0, 1);
	field_add(&field, 1, 2);
	field_add(&field, 2, 0);
	field_add(&field, 2, 1);
	field_add(&field, 2, 2);
	*/
	// the 5x5 infinite pattern
	field_add(&field, 47, 47); //field[47][47] = 1;
	field_add(&field, 47, 48); //field[47][48] = 1;
	field_add(&field, 47, 49); //field[47][49] = 1;
	field_add(&field, 47, 51); //field[47][51] = 1;
	field_add(&field, 48, 47); //field[48][47] = 1;
	field_add(&field, 49, 50); //field[49][50] = 1;
	field_add(&field, 49, 51); //field[49][51] = 1;
	field_add(&field, 50, 48); //field[50][48] = 1;
	field_add(&field, 50, 49); //field[50][49] = 1;
	field_add(&field, 50, 51); //field[50][51] = 1;
	field_add(&field, 51, 47); //field[51][47] = 1;
	field_add(&field, 51, 49); //field[51][49] = 1;
	field_add(&field, 51, 51); //field[51][51] = 1;

	printf("Cyleway's Game of Life\n");
	display(&field, X_SIZE, Y_SIZE);

	for (int i = 0; i < 50; i += 1) {
		printf("Generation %d, %d cells alive\n", i, livecnt);
		display_json(&field, X_SIZE, Y_SIZE);
		//display(&field, X_SIZE, Y_SIZE);
		age(&field, X_SIZE, Y_SIZE);
	}
}
