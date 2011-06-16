.global task_syscall
.global task_irq

.global get_user_psr
.type get_user_psr, %function
@ int get_user_psr()
get_user_psr:
	mrs   r0, cpsr
	@ Set psr to user mode
	bic   r0, r0, #0x1f
#ifdef SUPERVISOR_TASKS
	orr   r0, r0, #0x13
#else
	orr   r0, r0, #0x10
#endif
	@ Enable IRQs, enable FIQs
	bic   r0, r0, #0xc0	@ 0x80=IRQ disable, 0x40=FIQ disable
	@ Clear condition codes
	bic   r0, r0, #0xf0000000
	bx    lr

irq_stack:
	.word 0
	.word 0
	.word 0
	.word 0
irq_stack_ptr:
	.word irq_stack

.global init_cpumodes
.type init_cpumodes, %function
@ void init_cpumodes()
init_cpumodes:
	mrs   r0, cpsr
	bic   r0, r0, #0x40	@ 0x40=FIQ disable
	bic   r1, r0, #0x1f
	ldr   r3, irq_stack_ptr
	@ FIQ mode
	orr   r2, r1, #0x11
	msr   cpsr_c, r2
	mov   sp, r3
	@ IRQ mode
	orr   r2, r1, #0x12
	msr   cpsr_c, r2
	mov   sp, r3
	@ Return to supervisor mode
	msr   cpsr_c, r0
	bx    lr

#ifdef SUPERVISOR_TASKS
curkstack:
	.word 0
#endif

.global task_activate
.type task_activate, %function
@ task_activate(struct task *task)
task_activate:
	@ Store all of our non-temporary registers
	stmfd sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
	@ Load spsr
	ldr   r1, [r0, #64]
	msr   spsr_cxsf, r1
	@ Stash r0 (task) on our stack for exception entry
	str   r0, [sp, #-4]!
	@ Load user registers
	add   r1, r0, #12
#ifdef SUPERVISOR_TASKS
	str   sp, curkstack
	ldmia r1, {r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}
#else
	ldmia r1, {r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
	@ No banked register access after this, so no nop required
#endif
	@ Load pc and spsr
	ldmia r0, {r0, r1, pc}^
swi_enter:
#ifdef SUPERVISOR_TASKS
	@ Stick stuff on the user stack
	stmdb sp, {sl, fp, ip, sp}
	mov   sl, sp
	@ Grab task struct from kernel stack
	ldr   sp, curkstack
	ldr   fp, [sp], #4
	mov   ip, fp
	stmia ip!, {r0, r1, lr}
	stmia ip!, {r2, r3, r4, r5, r6, r7, r8, r9}
	ldmdb sl, {r0, r1, r2, r3}
	stmia ip!, {r0, r1, r2, r3}
	mov   r0, fp
	@ Restore some of our registers
	ldmfd sp, {r4, r5, r6, r7, r8, r9, sl, fp, ip}
#else
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
#endif
	@ Store spsr
	mrs   r1, spsr
	str   r1, [r0, #64]

	@ Set up args for syscall
	mov   r1, r0			@ task struct
	mov   r0, lr
	ldr   r0, [r0, #-4]		@ instruction contains swi
	bic   r0, r0, #0xff000000	@ decode swi value

	@ handle syscall
	@ void task_syscall(int code, struct task *task)
	bl    task_syscall

	@ Restore our registers
	ldmfd sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
	@ Return to main
	bx    lr
irq_enter:
	@ Fixup user saved pc (irq handler sets lr = pc+4)
	sub   lr, lr, #4
	@ Stash registers (r0, r1 for scratch, lr since its only available in this mode)
	stmia sp, {r0, r1, lr}
	@ Grab spsr
	mrs   r1, spsr
	@ Warp to supervisor mode
	mrs   r0, cpsr
	@bic   r0, r0, #0x1f @ Unnecessary because we are in IRQ or FIQ mode (0x11 or 0x12).
	orr   r0, r0, #0x13
	msr   cpsr_c, r0
#ifdef SUPERVISOR_TASKS
	@ Restore kernel stack
	ldr   r0, curkstack
	@ Stash spsr temporarily
	str   r1, [r0, #-4]
	@ Grab task struct
	ldr   r1, [r0]
	@ Stash user registers
	add   r1, r1, #12	@ skip r0, r1, pc
	stmia r1, {r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}
	mov   sp, r0
	sub   r0, r1, #12
	@ Store spsr
	ldr   r1, [sp, #-4]
	str   r1, [r0, #64]
#else
	@ Grab task struct from r0
	ldr   r0, [sp]
	@ Store spsr
	str   r1, [r0, #64]
	@ Stash user registers
	add   r1, r0, #12	@ skip r0, r1, pc
	stmia r1, {r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
	nop	@ needed to access banked regs after stm ^
#endif
	@ Stash user r0, r1 and return address
	ldr   r2, irq_stack_ptr
	ldmia r2, {r3, r4, r5} @ user r0, r1 and pc (saved lr in irq mode)
	stmia r0, {r3, r4, r5}
	@ We've got r0, so adjust the stack past it
	add   sp, sp, #4

#ifdef SUPERVISOR_TASKS
	@ Restore our registers
	ldmfd sp, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
#endif

	@ handle irq
	@ void long task_irq()
	bl    task_irq

	@ Restore our registers
	ldmfd sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
	@ Return to main
	bx    lr

@ PRM reset.
.global prm_reset
prm_reset:
	ldr r0, prm_rstctrl
	mov r1, #6 @ RST_DPLL3 | RST_GS
	push {r10}
	str r1, [r0]
prm_rstctrl:
	.word 0x48307250 @ PRM_RSTCTRL

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
	.word irq_enter
vec_fiq_addr:
	.word prm_reset
