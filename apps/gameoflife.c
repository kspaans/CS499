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

#define X_SIZE 10//0
#define Y_SIZE 10//0
#define HASH_SIZE 101
#define MTU 150

#define UDP_SRCPORT 8889
#define UDP_DSTPORT UDP_SRCPORT
#define GUMSTIX_CS IP(10, 0, 0, 1)

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

/*
 * Put (0,0) cell in the upper-left
 */
static void display(hashtable *field, size_t x, size_t y)
{
	void *state;
	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			if (hashtable_get(field, life_hashfunc(i, j), &state) == HT_NOKEY) {
				printf("0 ");
			} else {
				printf("1 ");
			}
		}
		printf("\n");
	}
}

/*
 * To be more frugal, send only the live cells to the other end.
 */
static void display_json(hashtable *field, size_t x, size_t y)
{
	char buf[MTU] = {};
	size_t idx = 0;
	void *state;
	uint8_t flag_firstloop = 1;

	sprintf(buf, "[");
	idx += 1;
	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			if (hashtable_get(field, life_hashfunc(i, j), &state) != HT_NOKEY) {
				if (!flag_firstloop) {
					sprintf(buf + idx, ",");
					idx += 1;
					if (idx >= MTU) goto error_send;
				} else {
					flag_firstloop = 0;
				}
				sprintf(buf + idx, "[%d,%d]", i, j);
				idx += 5;
				if (i >= 10)   idx += 1;
				if (j >= 10)   idx += 1;
				if (i >= 100)  idx += 1;
				if (j >= 100)  idx += 1;
				if (i >= 1000) idx += 1;
				if (j >= 1000) idx += 1;
				if (idx >= MTU) goto error_send;
			}
		}
	}
	sprintf(buf + idx, "]");
	idx += 1;
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
static uint8_t surround(hashtable *field, size_t curx, size_t cury, size_t x, size_t y)
{
	uint8_t count = 0;
	int8_t  leftmost = curx - 1;
	int8_t  topmost  = cury - 1;
	void *state;

	if (leftmost < 0) {
		leftmost = x - 1;
	}
	if (topmost < 0) {
		topmost = y - 1;
	}

	if (hashtable_get(field, life_hashfunc(leftmost, topmost), &state) != HT_NOKEY) { ++count; }
	if (hashtable_get(field, life_hashfunc(leftmost, (topmost + 1) % y), &state) != HT_NOKEY) { ++count; }
	if (hashtable_get(field, life_hashfunc(leftmost, (topmost + 2) % y), &state) != HT_NOKEY) { ++count; }
	if (hashtable_get(field, life_hashfunc((leftmost + 1) % x, topmost), &state) != HT_NOKEY) { ++count; }
	if (hashtable_get(field, life_hashfunc((leftmost + 1) % x, (topmost + 2) % y), &state) != HT_NOKEY) { ++count; }
	if (hashtable_get(field, life_hashfunc((leftmost + 2) % x, topmost), &state) != HT_NOKEY) { ++count; }
	if (hashtable_get(field, life_hashfunc((leftmost + 2) % x, (topmost + 1) % y), &state) != HT_NOKEY) { ++count; }
	if (hashtable_get(field, life_hashfunc((leftmost + 2) % x, (topmost + 2) % y), &state) != HT_NOKEY) { ++count; }
	return count;
}

/*
 * A little bit gross, we need access to the hash table store so that it can be
 * copied -- this gives us an unchanging copy of the field with which to use
 * while computing the next generation.
 */
static void age(hashtable *field, struct ht_item *uni, size_t x, size_t y)
{
	struct ht_item copy[HASH_SIZE];
	hashtable hcopy;
	uint8_t count;

	memcpy(copy, uni, sizeof(copy));
	memcpy(&hcopy, field, sizeof(hashtable));
	hcopy.arr = copy;

	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			count = surround(&hcopy, i, j, x, y);
			if (count > 3 || count < 2) {
				hashtable_del(field, life_hashfunc(i, j)); // cell dies
			} else if (count == 3) {
				hashtable_put(field, life_hashfunc(i, j), (void *) 1); // cell is born
			}
		}
	}
}

void gameoflife(void)
{
	//uint8_t *field[X_SIZE];
	//uint8_t universe[X_SIZE][Y_SIZE] = {};
	hashtable field;
	struct ht_item universe[HASH_SIZE];

	hashtable_init(&field, universe, HASH_SIZE, NULL, NULL);
	// start it with a glider
	hashtable_put(&field, life_hashfunc(0, 1), (void *)1); //field[0][1] = 1;
	hashtable_put(&field, life_hashfunc(1, 2), (void *)1); //field[1][2] = 1;
	hashtable_put(&field, life_hashfunc(2, 0), (void *)1); //field[2][0] = field[2][1] = field[2][2] = 1;
	hashtable_put(&field, life_hashfunc(2, 1), (void *)1);
	hashtable_put(&field, life_hashfunc(2, 2), (void *)1);

	/*
	// the 5x5 infinite pattern
	field[47][47] = 1;
	field[47][48] = 1;
	field[47][49] = 1;
	field[47][51] = 1;
	field[48][47] = 1;
	field[49][50] = 1;
	field[49][51] = 1;
	field[50][48] = 1;
	field[50][49] = 1;
	field[50][51] = 1;
	field[51][47] = 1;
	field[51][49] = 1;
	field[51][51] = 1;
	*/

	printf("Cyleway's Game of Life\n");

	for (int i = 0; i < 50; i += 1) {
		printf("Generation %d\n", i);
		display_json(&field, X_SIZE, Y_SIZE);
		display(&field, X_SIZE, Y_SIZE);
		age(&field, universe, X_SIZE, Y_SIZE);
	}
}
