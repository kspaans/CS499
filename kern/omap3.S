.section .init, "ax", %progbits

.global omap3_start
omap3_start:
	stmfd sp!, {fp, lr}
	mov fp, #0
	bl main
	ldmfd sp!, {fp, lr}
	bx lr
