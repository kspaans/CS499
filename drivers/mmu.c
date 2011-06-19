/*
 * Helper routines for dealing with the MMU
 */

#include <types.h>
#include <drivers/cpu.h>
#include <drivers/mmu.h>
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
void prep_pagetable()
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

/*
uint32_t ttb;
        asm ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttb));
printk("ttb = %08x\n", ttb);
	asm ("mrc p15, 0, %0, c1, c0, 0" : "=r" (ttb));
printk("MMU = %08x\n", ttb);
	asm ("MRC p15, 0, %0, c3, c0, 0" : "=r" (ttb));
printk("DACR = %08x\n", ttb);
	asm ("MRC p15, 0, %0, c0, c1, 4" : "=r" (ttb));
printk("MEM MODEL FEATURE 0 = %08x\n", ttb);
	asm ("MRC p15, 0, %0, c0, c1, 5" : "=r" (ttb));
printk("MEM MODEL FEATURE 1 = %08x\n", ttb);
	asm ("MRC p15, 0, %0, c0, c1, 6" : "=r" (ttb));
printk("MEM MODEL FEATURE 2 = %08x\n", ttb);
	asm ("MRC p15, 0, %0, c0, c1, 7" : "=r" (ttb));
printk("MEM MODEL FEATURE 3 = %08x\n", ttb);
	asm ("MRC p15, 0, %0, c0, c0, 3" : "=r" (ttb));
printk("TLB Type Reigster = %08x\n", ttb);
*/
}
