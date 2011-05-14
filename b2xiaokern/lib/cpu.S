.global init_cache
.type init_cache, %function
init_cache:
	@ read p15c1 (control reg)
	mrc p15, 0, r1, c1, c0
	orr r1, r1, #0x4	@ enable data cache
	orr r1, r1, #0x1000	@ enable instruction cache
	@ write p15c1 (control reg)
	mcr p15, 0, r1, c1, c0
	bx  lr
