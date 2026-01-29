ENTRY(_start)
_kernel_start = 0x20000000;
SECTIONS
{
	. = _kernel_start;
	.init : { *(.init) }
	.text : { *(.text) }
	*(.*)
	. = ALIGN(1M);  /* Force end to a 1MB boundary for the MMU */
	_kernel_end = .;
	ASSERT(_kernel_end - _kernel_start <= 1M, "Error: Kernel exceeds 1MB MMU allocation!")
}
