.macro SET_STACK mode, stack
	mrs r0, cpsr @ Read CPSR, keep the old cpsr
	bic r1, r0, #0x1F @ Modify by removing current mode
	orr r1, r1, \mode @ and substitute it with mode
	msr cpsr_c, r1 @ Change the mode

	ldr sp, =\stack

	msr cpsr_c, r0 @ restore the original mode
.endm

.section .init
.global _start
_start:
    @ remap memory
    ldr r0, =REMAP_CONTROL_REGISTER
    ldr r1, =REMAP_COMMAND_BIT
    STR  R1, [R0]
    @ fiq mode
    SET_STACK #MODE_FIQ, __stack_top_fiq
    @ irq mode
    SET_STACK #MODE_IRQ, __stack_top_irq
    @ supervisor mode
    SET_STACK #MODE_SUPERVISOR, __stack_top_supervisor
    @ abort mode
    SET_STACK #MODE_ABORT, __stack_top_abort
    @ undefined mode
    SET_STACK #MODE_UNDEFINED, __stack_top_undefined
    @ system mode
    SET_STACK #MODE_SYSTEM, __stack_top_system

    bl init_exceptions
    bl main

.end:
    b .end

.equ MODE_FIQ, 0b10001
.equ MODE_IRQ, 0b10010
.equ MODE_SUPERVISOR, 0b10011
.equ MODE_ABORT, 0b10111
.equ MODE_UNDEFINED, 0b11011
.equ MODE_SYSTEM, 0b11111
.equ MODE_USER, 0b10000
.equ INTERRUPT_BIT, 1 << 7
.equ FINTERRUPT_BIT, 1 << 6
.equ INTERRUPTS_DISABLED, (INTERRUPT_BIT | FINTERRUPT_BIT) @ All modes start disabled

.equ REMAP_CONTROL_REGISTER, 0xFFFFFF00
.equ REMAP_COMMAND_BIT, 1 << 0
