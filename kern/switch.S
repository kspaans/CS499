#include <arm.h>

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

.global init_cpumodes
.type init_cpumodes, %function
@ void init_cpumodes(void)
init_cpumodes:
	@ setup stack poison
	ldr   r0, =0x55555555

	@ FIQ mode
	cps   #ARM_MODE_FIQ
	mov   sp, r0

	@ IRQ mode
	cps   #ARM_MODE_IRQ
	mov   sp, r0

	@ ABT mode
	cps   #ARM_MODE_ABT
	mov   sp, r0

	@ UND mode
	cps   #ARM_MODE_UND
	mov   sp, r0

	@ Return to supervisor mode
	cps   #ARM_MODE_SVC
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

.macro ex_handler name lr_off

\name\()_enter:
	@ Fixup saved program counter
	sub   lr, lr, #\lr_off

	@ Save return state to SVC stack
	srsdb sp!, #ARM_MODE_SVC

	@ Verify the abort happened in USR mode
	mrs   lr, spsr
	and   lr, lr, #ARM_MODE_MASK
	cmp   lr, #ARM_MODE_USR
	bne   \name\()_svc

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
	@ void task_foo(struct task *)
	b     task_\name

\name\()_svc:
	@ Warp to SVC mode
	cps   #ARM_MODE_SVC

	@ Save SVC registers
	sub   sp, sp, #60
	stmia sp, {r0 - r14}

	@ Fixup stack pointer in struct regs (+sizeof(struct regs))
	add   r0, sp, #68
	str   r0, [sp, #52]

	@ Save pointer to start of struct regs
	mov   r0, sp

	@ Call kernel_foo(struct regs *)
	b     kernel_\name
.endm

ex_handler irq  4
ex_handler pabt 4
ex_handler dabt 8
ex_handler und  4

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
	.word und_enter
vec_swi_addr:
	.word swi_enter
vec_prefetch_abort_addr:
	.word pabt_enter
vec_data_abort_addr:
	.word dabt_enter
vec_unused_addr:
	.word prm_reset
vec_irq_addr:
	.word irq_enter
vec_fiq_addr:
	.word prm_reset
