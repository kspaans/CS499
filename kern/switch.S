#include <arm.h>

.global task_syscall
.global task_irq

.global get_user_psr
.type get_user_psr, %function
@ int get_user_psr(void)
get_user_psr:
	mrs   r0, cpsr

	@ Set psr to user mode
	bic   r0, r0, #0x1f
	orr   r0, r0, #0x10

	@ Enable IRQs, enable FIQs
	bic   r0, r0, #0xc0	@ 0x80=IRQ disable, 0x40=FIQ disable

	@ Clear condition codes
	bic   r0, r0, #0xf0000000
	bx    lr

irq_stack:
	.space 16

.global init_cpumodes
.type init_cpumodes, %function
@ void init_cpumodes(void)
init_cpumodes:

	@ setup
	mrs   r0, cpsr
	bic   r0, r0, #0x40	@ 0x40=FIQ disable
	bic   r1, r0, #ARM_MODE_MASK
	ldr   r3, =irq_stack

	@ FIQ mode
	orr   r2, r1, #ARM_MODE_FIQ
	msr   cpsr_c, r2
	mov   sp, r3

	@ IRQ mode
	orr   r2, r1, #ARM_MODE_IRQ
	msr   cpsr_c, r2
	mov   sp, r3

	@ ABT mode
	orr   r2, r1, #ARM_MODE_ABT
	msr   cpsr_c, r2
	mov   sp, r3

	@ UND mode
	orr   r2, r1, #ARM_MODE_UND
	msr   cpsr_c, r2
	mov   sp, r3

	@ Return to supervisor mode
	msr   cpsr_c, r0
	bx    lr

.global task_activate
.type task_activate, %function
@ task_activate(struct task *task)
task_activate:

	@ Store all of our non-temporary registers, plus r0 (task)
	push  {r0, r4 - r11, r14}

	@ Calculate offset of {pc, psr} for exception return
	add   lr, r0, #60

	@ Load user registers
	ldmia r0, {r0 - r14}^

	@ Exception return
	rfeia lr

swi_enter:
	@ Stack user pc, load task pointer
	str   lr, [sp, #-4]!
	ldr   lr, [sp, #4]

	@ Stash user registers
	stmia lr, {r0 - r14}^

	@ Store user psr
	mrs   r0, spsr
	str   r0, [lr, #64]

	@ Store user pc
	ldr   r0, [sp], #4
	str   r0, [lr, #60]

	@ Restore kernel registers
	pop   {r0, r4 - r11, lr}

	@ handle syscall (tail call)
	@ void task_syscall(struct task *task)
	b     task_syscall

irq_enter:
	@ Fixup user saved pc (irq handler sets lr = pc + 4)
	sub   lr, lr, #4

	@ Save return state to SVC stack
	srsdb sp!, #ARM_MODE_SVC

	@ Warp to SVC mode
	cps   #ARM_MODE_SVC

	@ Grab task struct from stack
	ldr   lr, [sp, #8]

	@ Save user registers
	stmia lr, {r0 - r14}^

	@ Save user pc, psr
	pop   {r0 - r1}
	str   r0, [lr, #60]
	str   r1, [lr, #64]

	@ Restore kernel registers
	pop   {r0, r4 - r11, lr}

	@ handle irq
	@ void task_irq(void)
	b     task_irq

@ PRM reset.
.global prm_reset
prm_reset:
	ldr r0, prm_rstctrl
	mov r1, #6 @ RST_DPLL3 | RST_GS
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
	.word panic_undef
vec_swi_addr:
	.word swi_enter
vec_prefetch_abort_addr:
	.word panic_pabt
vec_data_abort_addr:
	.word panic_dabt
vec_unused_addr:
	.word panic_unused
vec_irq_addr:
	.word irq_enter
vec_fiq_addr:
	.word prm_reset
