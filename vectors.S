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
	.word swi_handler
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
