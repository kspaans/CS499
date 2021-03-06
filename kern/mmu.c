/*
 * Helper routines for dealing with the MMU
 */

#include <types.h>
#include <kern/cpu.h>
#include <kern/mmu.h>
#include <lib.h>
#include <string.h> // memset
#include <kern/printk.h>

uint32_t pagetable[0x1000] __attribute__((aligned(0x4000)));

// 16 * 1024 needed for the page table, 14 bits of stuff see docs/mmu.txt for
// which addresses need to be in the table
/*
 * Prepare the page table for the MMU by allocating space for the supersection
 * first-level descriptors. Importnant to note the layout described here, that
 * the FLDs have to be repeated (due to overlaping index bits, see docs).
 */
void prep_pagetable(void)
{
	uint32_t i;

	memset(pagetable, 0, sizeof(pagetable)); // start will invalid FLDs -- all 0s

	// The top 14 bits of the VA make up the table index, used here to add
	// the entries and their replicas.
	for (i = 0; i < 16; i += 1) {
		for(int j=0; j<0x80; j++) {
			pagetable[j*16 + i] = (j<<24) | FLD_FLAGS_DEVICEMEM;
		}
		for(int j=0x80; j<0x100; j++) {
			pagetable[j*16 + i] = (j<<24) | FLD_FLAGS_CACHEABLE;
		}
	}

	printk("pagetable init\n");
	set_ttbr(pagetable);
	set_dacr(0x3); // domain 0 manager access
}
