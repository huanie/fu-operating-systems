ENTRY(_start)

MEMORY
{
	RAM (rwx) : ORIGIN = 0x00200000, LENGTH = 16K
}
__stack_size_exc = 0x400;
__stack_top = ORIGIN(RAM) + LENGTH(RAM);
__stack_end = ORIGIN(RAM);

__stack_top_fiq = __stack_top;
__stack_top_irq = __stack_top_fiq - __stack_size_exc;
__stack_top_abort = __stack_top_irq - __stack_size_exc;
__stack_top_undefined = __stack_top_abort - __stack_size_exc;
__stack_top_supervisor = __stack_top_undefined - __stack_size_exc;

/* system and user stack are shared */
__stack_top_system = __stack_top_supervisor - __stack_size_exc;
__stack_top_user =  __stack_top_system;


SECTIONS
{
	. = 0x20000000;
	.init : { *(.init) }
	.text : { *(.text) }
}
