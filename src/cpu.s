.global disable_irq
disable_irq:
	mrs r0, cpsr
	orr r0, r0, #(1 << 7)
	msr cpsr_c, r0
	mov pc, lr


.global enable_irq
enable_irq:
	mrs r0, cpsr
	bic r0, r0, #(1 << 7)
	msr cpsr_c, r0
	mov pc, lr


.global irq_disabled
irq_disabled:
	mrs r0, cpsr
	and r0, r0, #(1 << 7)
	mov pc, lr
