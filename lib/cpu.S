.global init_cache
.type init_cache, %function
init_cache:
	@ enable L1 cache
	@ read p15c1 (control reg)
	mrc p15, 0, r1, c1, c0, 0
	orr r1, r1, #0x4	@ enable data cache
	orr r1, r1, #0x1000	@ enable instruction cache
	@ write p15c1 (control reg)
	mcr p15, 0, r1, c1, c0, 0

	@ enable L2 cache
	@ read p15c1 (control reg)
	mrc p15, 0, r1, c1, c0, 1
	orr r1, r1, #0x2	@ enable L2 cache
	@ write p15c1 (control reg)
	mcr p15, 0, r1, c1, c0, 1

	bx  lr

.global init_mmu
.type init_mmu, %function
init_mmu:
	mrc p15, 0, r1, c1, c0, 0 @ read CP15 Register 1
	orr r1, r1, #0x1 	  @ bit 0 to 1 to enable MMU
	mcr p15, 0, r1, c1, c0, 0 @ enable MMUs
	bx lr

.global deinit_mmu
.type deinit_mmu, %function
deinit_mmu:
	mrc p15, 0, r1, c1, c0, 0 @ read CP15 Register 1
	and r1, r1, #0xFFFFFFFE	  @ bit 0 to 0 to disable MMU
	mcr p15, 0, r1, c1, c0, 0 @ disable MMUs
	bx lr

@ First argument should be a pointer to the page table
.global set_ttbr
.type set_ttbr, %function
set_ttbr:
	@mrc p15, 0, r1, c2, c0, 0 @ read TTBR0, upper 14 bits are pointer, leave the rest to be zero
	mcr p15, 0, r0, c2, c0, 0 @ write page table pointer, it should be aligned to a 16KB boundary
	mov r1, #0xFF
	mcr p15, 0, r1, c3, c0, 0 @ set DACR, so that the domain is readable
	bx lr
