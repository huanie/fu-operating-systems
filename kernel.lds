ENTRY(_start)
_kernel_start = 0x20000000;
SECTIONS
{
	. = _kernel_start;
	.init : { *(.init) }
	.text : { *(.text) }
}
