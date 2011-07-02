#ifndef KERN_MMU_H
#define KERN_MMU_H

/* First-Level Descriptor Flags */
#define FLD_TYPE_SECTION  0x2
#define FLD_TYPE_SUPERSEC (1 << 18)
#define FLD_NONSECURE     (1 << 19)
#define FLD_NOTGLOBAL     (1 << 17)
#define FLD_SHAREABLE     (1 << 16)
#define FLD_AP0           (1 << 10)
#define FLD_AP1           (1 << 11)
#define FLD_AP2           (1 << 15)
#define FLD_TEX0          (1 << 12)
#define FLD_TEX1          (1 << 13)
#define FLD_TEX2          (1 << 14)
#define FLD_EXECNEVER     (1 <<  4)
#define FLD_CACHEABLE     (1 <<  3)
#define FLD_BUFFERABLE    (1 <<  2)

/*
 * "Memory Region Attributes" controlled by the TEX, C, B bits
 * Assume that the TEX remap is disabled in the system control register
 */
#define FLD_MEMREGATTR_STRONGLYORDERED 0 // strongly-ordered memory
#define FLD_MEMREGATTR_NONSHAREABLEDEV (FLD_TEX1) // Non-shareable Device memory
#define FLD_MEMREGATTR_NORMALWBWA      (FLD_TEX0 | FLD_CACHEABLE | FLD_BUFFERABLE) // Normal memory, Outer and Inner Write-Back, Write-Allocate
#define FLD_MEMREGATTR_CACHEABLEWBWA   (FLD_TEX2 | FLD_TEX0 | FLD_BUFFERABLE) // Cacheable Normal memory, Write-Back, Write-Allocate
#define FLD_ACCESS_RW                  (FLD_AP1 | FLD_AP0) // Memory access permissions: read/write in all modes

/* All of the flags necessary for a FLD */
#define FLD_FLAGS_CACHEABLE FLD_TYPE_SECTION | FLD_TYPE_SUPERSEC | FLD_NONSECURE | \
        FLD_NOTGLOBAL | FLD_MEMREGATTR_CACHEABLEWBWA | FLD_ACCESS_RW
#define FLD_FLAGS_DEVICEMEM FLD_TYPE_SECTION | FLD_TYPE_SUPERSEC | FLD_NONSECURE | \
        FLD_NOTGLOBAL | FLD_MEMREGATTR_STRONGLYORDERED | FLD_ACCESS_RW

void prep_pagetable(void);

#endif /* KERN_MMU_H */
