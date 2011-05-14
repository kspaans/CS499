.section .text, "ax", %progbits
.global kern_syscall

.global get_user_psr
.type get_user_psr, %function
@ int get_user_psr()
get_user_psr:
	mrs   r0, cpsr
	@ Set psr to system mode
	@ bic   r0, r0, #0x1f
	orr   r0, r0, #0x1f
	@ *disable* interrupts
	@ TODO: when interrupts work, enable them here!
	bic   r0, r0, #0xc0
	orr   r0, r0, #0xc0	@ 0x80=IRQ disable, 0x40=FIQ disable
	@ clear condition codes
	bic   r0, r0, #0xf0000000
	bx    lr

.global task_activate
.type task_activate, %function
@ task_activate(struct task *task)
task_activate:
	@ Store all of our non-temporary registers
	stmfd sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
	@ Load spsr
	ldr   r1, [r0, #64]
	msr   spsr_cxsf, r1
	@ Load user registers
	add   r1, r0, #12	@ skip r0, r1, pc
	ldmia r1, {r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
	nop	@ needed to access banked regs after ldm ^
	@ Stash r0 (task) on our stack for exception entry
	str   r0, [sp, #-4]!
	@ Load pc and spsr
	ldmia r0, {r0, r1, pc}^
swi_enter:
	@ Give ourselves some temporary room
	stmdb sp, {r0, r1}
	@ Grab task struct from r0
	ldr   r0, [sp]
	@ Stash user registers
	add   r1, r0, #12	@ skip r0, r1, pc
	stmia r1, {r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
	nop	@ needed to access banked regs after stm ^
	@ Stash user r0, r1 and return address
	ldmdb sp, {r2, r3}
	stmia r0, {r2, r3, lr}
	@ We've got r0, so adjust the stack past it
	add   sp, sp, #4
	@ Store spsr
	mrs   r1, spsr
	str   r1, [r0, #64]

	@ Set up args for syscall
	mov   r1, r0			@ task struct
	mov   r0, lr
	ldr   r0, [r0, #-4]		@ instruction contains swi
	bic   r0, r0, #0xff000000	@ decode swi value

	@ handle syscall
	@ void kern_syscall(int code, struct task *task)
	bl    kern_syscall

	@ Restore our registers
	ldmfd sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
	@ Return to main
	bx    lr

.section .vectors, "ax", %progbits
vec_undefined:
	ldr pc, vec_undefined_addr
vec_swi:
	ldr pc, vec_swi_addr
vec_prefetch_abort:
	ldr pc, vec_prefetch_abort_addr
vec_data_abort:
	ldr pc, vec_data_abort_addr
vec_unused:
	ldr pc, vec_unused_addr
vec_irq:
	ldr pc, vec_irq_addr
vec_fiq:
	ldr pc, vec_fiq_addr

vec_undefined_addr:
	.word 0x14080
vec_swi_addr:
	.word swi_enter
vec_prefetch_abort_addr:
	.word 0x14088
vec_data_abort_addr:
	.word 0x1408c
vec_unused_addr:
	.word 0x14090
vec_irq_addr:
	.word 0x14094
vec_fiq_addr:
	.word 0x14098
