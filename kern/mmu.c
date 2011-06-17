/*
 * Helper routines for dealing with the MMU
 */

#include <types.h>
#include <mmu.h>
#include <kern/kmalloc.h>
#include <lib.h>
#include <string.h> // memset
#include <kern/printk.h>


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

uint32_t pagetable[0x4000] __attribute__((aligned(0x4000)));

// 16 * 1024 needed for the page table, 14 bits of stuff see docs/mmu.txt for
// which addresses need to be in the table
/*
 * Prepare the page table for the MMU by allocating space for the supersection
 * first-level descriptors. Importnant to note the layout described here, that
 * the FLDs have to be repeated (due to overlaping index bits, see docs).
 */
void prep_pagetable()
{
	uint32_t i;

	printk("PAGETABLE AT %p\r\n", pagetable);
	memset(pagetable, 0, 16 * 1024); // start will invalid FLDs -- all 0s

	// The top 14 bits of the VA make up the table index, used here to add
	// the entries and their replicas.
	pagetable[0x1000] = FLD40;
	for (i = 1; i < 16; i += 1) {
		pagetable[0x1000 + i] = FLD40;
	}
	pagetable[0x2000] = FLD80;
	for (i = 1; i < 16; i += 1) {
		pagetable[0x2000 + i] = FLD80;
	}
	pagetable[0x23c0] = FLD8F;
	for (i = 1; i < 16; i += 1) {
		pagetable[0x23c0 + i] = FLD8F;
	}
	pagetable[0x1200] = FLD48;
	for (i = 1; i < 16; i += 1) {
		pagetable[0x1200 + i] = FLD48;
	}
	pagetable[0x1240] = FLD49;
	for (i = 1; i < 16; i += 1) {
		pagetable[0x1240 + i] = FLD49;
	}
	pagetable[0x0ac0] = FLD2B;
	for (i = 1; i < 16; i += 1) {
		pagetable[0x0ac0 + i] = FLD2B;
	}
	pagetable[0x0b00] = FLD2C;
	for (i = 1; i < 16; i += 1) {
		pagetable[0x0b00 + i] = FLD2C;
	}
	memset(pagetable, 0, 16 * 1024); // start will invalid FLDs -- all 0s

	set_ttbr(pagetable);

uint32_t ttb;
        asm ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttb));
printk("ttb = %08x\n", ttb);


}
