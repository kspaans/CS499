	.align	2
	.global	memcpy
	.type	memcpy, %function
@ void memcpy(char *dest, const char *src, int len)
memcpy:
	@ if len is zero, don't copy anything
	cmp	r2, #0
	bxeq	lr
	@ check (int)dest & 3
	tst	r0, #3
	bne	.byteloop
	@ check (int)src & 3
	tst	r1, #3
	beq	.alignedcopy
.byteloop:
	@ copy single bytes at a time
	ldrb	r3, [r1], #1
	strb	r3, [r0], #1
	subs	r2, r2, #1
	bne	.byteloop
	bx	lr
.alignedcopy:
	@ copy the aligned region (up to the possibly unaligned tail)
	stmfd	sp!, {r0, r1, r2, lr}
	mov	r2, r2, asr #2
	bl	copy_aligned_region(PLT)
	ldmfd	sp!, {r0, r1, r2, lr}
	@ check for an unaligned tail
	bic	r3, r2, #3
	ands	r2, r2, #3
	bxeq	lr
	add	r0, r0, r3
	add	r1, r1, r3
	b	.byteloop
	.size	memcpy, .-memcpy

	.align	2
	.global	copy_aligned_region
	.type	copy_aligned_region, %function
@ void copy_aligned_region(int *dest, const int *src, int len)
copy_aligned_region:
	cmp	r2, #15
	ble	.slowcopy

	stmfd sp!, {r3-r12}
.fastcopy:
	ldmia r1!, {r3-r12}
	stmia r0!, {r3-r12}
	sub	r2, r2, #10
	cmp	r2, #10
	bge	.fastcopy
	ldmfd sp!, {r3-r12}

.slowcopy:
	cmp r2, #0
	bxle lr
.slowloop:
	ldr r3, [r1], #4
	str r3, [r0], #4
	subs r2, r2, #1
	bne .slowloop
	bx lr
.size	copy_aligned_region, .-copy_aligned_region
