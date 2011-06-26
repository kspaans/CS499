/*
 * Kyle's simple application: Game of Life
 * use the standard 3/23 rule
 */
#include <types.h>
#include <ip.h>
#include <servers/net.h>
#include <string.h>
#include <lib.h>
#include <apps.h>

#define X_SIZE 10//0
#define Y_SIZE 10//0

#define UDP_SRCPORT 8889
#define UDP_DSTPORT UDP_SRCPORT
#define GUMSTIX_CS IP(10, 0, 0, 1)

/*
 * Put (0,0) cell in the upper-left
 */
static void display(uint8_t **field, size_t x, size_t y)
{
	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			printf("%d ", field[i][j]);
		}
		printf("\n");
	}
}

static void display_json(uint8_t **field, size_t x, size_t y)
{
	char buf[X_SIZE * Y_SIZE * 4] = {}; /* should be enough for the JSON */
	size_t idx = 0;

	sprintf(buf, "[");
	idx += 1;
	for (size_t i = 0; i < x; i += 1) {
		if (i) {
			sprintf(buf + idx, ",");
			idx += 1;
		}
		sprintf(buf + idx, "[");
		idx += 1;
		for (size_t j = 0; j < y; j += 1) {
			if (j) {
				sprintf(buf + idx, ",");
				idx += 1;
			}
			sprintf(buf + idx, "%d", field[i][j]);
			idx += 1; // all values should be 0 or 1
		}
		sprintf(buf + idx, "]");
		idx += 1;
	}
	sprintf(buf + idx, "]");
	idx += 1;
	send_udp(UDP_SRCPORT, GUMSTIX_CS, UDP_DSTPORT, buf, idx);
}

/*
 * Use wrap-around semantics for the edges of the field
 */
static uint8_t surround(uint8_t **field, size_t curx, size_t cury, size_t x, size_t y)
{
//	uint8_t count = 0;
	int8_t  leftmost = curx - 1;
	int8_t  topmost  = cury - 1;

	if (leftmost < 0) {
		leftmost = x - 1;
	}
	if (topmost < 0) {
		topmost = y - 1;
	}

	return (field[leftmost][topmost] +
		field[leftmost][(topmost + 1) % y] +
		field[leftmost][(topmost + 2) % y] +
		field[(leftmost + 1) % x][topmost] +
		field[(leftmost + 1) % x][(topmost + 2) % y] +
		field[(leftmost + 2) % x][topmost] +
		field[(leftmost + 2) % x][(topmost + 1) % y] +
		field[(leftmost + 2) % x][(topmost + 2) % y]);
}

static void age(uint8_t **field, size_t x, size_t y)
{
	uint8_t copy[x][y];
	uint8_t *cptrs[x];
	uint8_t count;

	for (size_t i = 0; i < x; i += 1) {
		cptrs[i] = copy[i];
	}
	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			cptrs[i][j] = field[i][j];
		}
	}

	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			count = surround(cptrs, i, j, x, y);
			if (count > 3 || count < 2) {
				field[i][j] = 0; // cell dies
			} else if (count == 3) {
				field[i][j] = 1; // cell is born
			}
		}
	}
}

void gameoflife(void)
{
	uint8_t *field[X_SIZE];
	uint8_t universe[X_SIZE][Y_SIZE] = {};

	for (size_t i = 0; i < X_SIZE; i += 1) {
		field[i] = universe[i];
	}
	// start it with a glider
	field[0][1] = 1;
	field[1][2] = 1;
	field[2][0] = field[2][1] = field[2][2] = 1;

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
		display_json(field, X_SIZE, Y_SIZE);
		//display(field, X_SIZE, Y_SIZE);
		age(field, X_SIZE, Y_SIZE);
	}
	return;
	display(field, X_SIZE, Y_SIZE);
	age(field, X_SIZE, Y_SIZE);
	printf("-- -- -- --\n");

	display(field, X_SIZE, Y_SIZE);
	display_json(field, X_SIZE, Y_SIZE);
	age(field, X_SIZE, Y_SIZE);
	printf("-- -- -- --\n");

	display(field, X_SIZE, Y_SIZE);
	display_json(field, X_SIZE, Y_SIZE);
	age(field, X_SIZE, Y_SIZE);
	printf("-- -- -- --\n");

	display(field, X_SIZE, Y_SIZE);
	display_json(field, X_SIZE, Y_SIZE);
	age(field, X_SIZE, Y_SIZE);
	printf("-- -- -- --\n");

	display(field, X_SIZE, Y_SIZE);
	display_json(field, X_SIZE, Y_SIZE);
}
