/*
 * Kyle's simple application: Game of Life
 * use the standard 3/23 rule
 */
#include <types.h>
#include <kern/printk.h>
#include <string.h>

#define X_SIZE 10
#define Y_SIZE 10

/*
 * Put (0,0) cell in the upper-left
 */
void display(uint8_t **field, size_t x, size_t y)
{
	for (size_t i = 0; i < x; i += 1) {
		for (size_t j = 0; j < y; j += 1) {
			printk("%d ", field[i][j]);
		}
		printk("\n");
	}
}

/*
 * Use warp-around semantics for the edges of the field
 */
uint8_t surround(uint8_t **field, size_t curx, size_t cury, size_t x, size_t y)
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

void age(uint8_t **field, size_t x, size_t y)
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

void gameoflife()
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

	printk("Cyleway's Game of Life\n");
	display(field, X_SIZE, Y_SIZE);
	age(field, X_SIZE, Y_SIZE);
	printk("-- -- -- --\n");
	display(field, X_SIZE, Y_SIZE);
	age(field, X_SIZE, Y_SIZE);
	printk("-- -- -- --\n");
	display(field, X_SIZE, Y_SIZE);
	age(field, X_SIZE, Y_SIZE);
	printk("-- -- -- --\n");
	display(field, X_SIZE, Y_SIZE);
	age(field, X_SIZE, Y_SIZE);
	printk("-- -- -- --\n");
	display(field, X_SIZE, Y_SIZE);
}
