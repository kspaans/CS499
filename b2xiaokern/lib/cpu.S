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
