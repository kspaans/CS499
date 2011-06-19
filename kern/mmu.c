/*
 * Helper routines for dealing with the MMU
 */

#include <types.h>
#include <mmu.h>
#include <kern/kmalloc.h>
#include <lib.h>
#include <string.h> // memset
#include <kern/printk.h>


#define FLD_CACHABLE
#define FLD_BUFFERABLE
#define FLD_SHAREABLE
#define FLD_EXECNEVER
#define FLD_GLOBAL
#define FLD_NEVERSHARE

// Page table entries -- "First Level Descriptors"  FLAGIFY ME
// Device memory
#define FLD48 0x480c0c06
#define FLD49 0x490c0c06
#define FLD2B 0x2b0c0c06
#define FLD2C 0x2c0c0c06
// Normal memory
#define FLD40 0x400c0c0a
#define FLD80 0x800c0c0a
#define FLD90 0x900c0c0a
#define FLD_FLAGS_DEVICE 0xc0c06
#define FLD_FLAGS_NORMAL 0xc0c0a

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

	// could have fucked up the "N" part for the TTBR#
	memset(pagetable, 0, sizeof(pagetable)); // start will invalid FLDs -- all 0s

	// The top 14 bits of the VA make up the table index, used here to add
	// the entries and their replicas.
	// ROBERT EDIT: Just map all memory. Lower 2GB is device, upper 2GB is RAM
	for (i = 0; i < 16; i += 1) {
		for(int j=0; j<0x80; j++) {
			pagetable[j*16 + i] = (j<<24) | FLD_FLAGS_DEVICE;
		}
		for(int j=0x80; j<0x100; j++) {
			pagetable[j*16 + i] = (j<<24) | FLD_FLAGS_DEVICE;
		}
		/*
		pagetable[0x0ac0 / 4 + i] = FLD2B;
		pagetable[0x0b00 / 4 + i] = FLD2C;
		pagetable[0x1000 / 4 + i] = FLD40;
		pagetable[0x1200 / 4 + i] = FLD48;
		pagetable[0x1240 / 4 + i] = FLD49;
		pagetable[0x2000 / 4 + i] = FLD80;
		pagetable[0x2400 / 4 + i] = FLD90;
		*/
	}

	//memset(pagetable, 0, sizeof(pagetable)); // start will invalid FLDs -- all 0s

	printk("pagetable init");
	set_ttbr(pagetable);

uint32_t ttb;
        asm ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttb));
printk("ttb = %08x\n", ttb);
	asm ("mrc p15, 0, %0, c1, c0, 0" : "=r" (ttb));
printk("MMU = %08x\n", ttb);
	asm ("MRC p15, 0, %0, c3, c0, 0" : "=r" (ttb));
printk("DACR = %08x\n", ttb);

}
