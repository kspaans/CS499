	.align  2
	.global memcpy
	.type   memcpy, %function
@ void *memcpy(void *d, const void *s, size_t size)
memcpy:
	cmp     r2, #0                  /* if len is zero, don't do anything */
	bxeq    lr
	push    {r0}                    /* save dst to return it later */
	tst     r0, #3                  /* check (int)dest & 3 */
	bne     .byteloop
	tst     r1, #3                  /* check (int)src & 3 */
	beq     .alignedcopy

.byteloop:
	ldrb    r3, [r1], #1            /* copy a byte at a time */
	strb    r3, [r0], #1
	subs    r2, r2, #1
	bne     .byteloop
	pop     {r0}
	bx      lr

.alignedcopy:                       /* copy the aligned region */
	subs    r2, r2, #40
	blo     .wordcopy               /* if copying <40 bytes, skip fastcopy */

	stmfd   sp!, {r3-r12}
.fastcopy:                          /* copy ten words at a time */
	ldmia   r1!, {r3-r12}
	stmia   r0!, {r3-r12}
	subs    r2, r2, #40
	bhs     .fastcopy               /* continue copying if we have >= 40 bytes left */
	ldmfd   sp!, {r3-r12}

.wordcopy:
	adds    r2, r2, #36             /* add 40 and subtract 4 */
	bmi     .unaligned_tail         /* if <4 bytes to go, skip word loop */

.wordloop:                          /* copy one word at a time */
	ldr     r3, [r1], #4
	str     r3, [r0], #4
	subs    r2, r2, #4
	bge     .wordloop

.unaligned_tail:                    /* <=3 bytes left */
	tst     r2, #3                  /* check for an unaligned tail */
	popeq   {r0}                    /* no tail: done */
	bxeq    lr
	add     r2, r2, #4              /* compensate for extra subtraction */
	b       .byteloop
.size   memcpy, .-memcpy
