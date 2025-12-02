.section .init
.global _start
_start:
    @ remap memory
    ldr r0, =REMAP_CONTROL_REGISTER
    ldr r1, =REMAP_COMMAND_BIT
    STR  R1, [R0]
    @ fiq mode
    msr cpsr_c, #MODE_FIQ
    ldr sp, =__stack_top_fiq
    @ irq mode
    msr cpsr_c, #MODE_IRQ
    ldr sp, =__stack_top_irq
    @ supervisor mode
    msr cpsr_c, #MODE_SUPERVISOR
    ldr sp, =__stack_top_supervisor
    @ abort mode
    msr cpsr_c, #MODE_ABORT
    ldr sp, =__stack_top_abort
    @ undefined mode
    msr cpsr_c, #MODE_UNDEFINED
    ldr sp, =__stack_top_undefined
    @ system mode
    msr cpsr_c, #MODE_SYSTEM
    ldr sp, =__stack_top_system
    bl init_interrupts
    @ user mode
    msr cpsr_c, #MODE_USER
    ldr sp, =__stack_top_user
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
