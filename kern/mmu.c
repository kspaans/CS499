/*
 * Helper routines for dealing with the MMU
 */

#include <types.h>
#include <mmu.h>
#include <kern/kmalloc.h>
#include <lib.h>
#include <string.h> // memset

// Page table entries -- "First Level Descriptors"
// Device memory
#define FLD48 0x480c0c06
#define FLD49 0x490c0c06
#define FLD2B 0x2b0c0c06
#define FLD2C 0x2c0c0c06
// Normal memory
#define FLD40 0x400c0c0a
#define FLD80 0x800c0c0a
#define FLD8F 0x8f0c0c0a

// 16 * 1024 needed for the page table, 14 bits of stuff see docs/mmu.txt for
// which addresses need to be in the table
/*
 * Prepare the page table for the MMU by allocating space for the supersection
 * first-level descriptors. Importnant to note the layout described here, that
 * the FLDs have to be repeated (due to overlaping index bits, see docs).
 */
void prep_mmu()
{
 	uint32_t *page_table = kmalloc(16 * 1024);
	uint32_t i;

	memset(page_table, 0, 16 * 1024); // start will invalid FLDs -- all 0s

	// The top 14 bits of the VA make up the table index, used here to add
	// the entries and their replicas.
	page_table[0x1000] = FLD40;
	for (i = 1; i < 16; i += 1) {
		page_table[0x1000 + i] = FLD40;
	}
	page_table[0x2000] = FLD80;
	for (i = 1; i < 16; i += 1) {
		page_table[0x2000 + i] = FLD80;
	}
	page_table[0x23c0] = FLD8F;
	for (i = 1; i < 16; i += 1) {
		page_table[0x23c0 + i] = FLD8F;
	}
}
