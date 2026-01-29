// WARNING: EDITING THIS ALSO REQUIRES EDITING start.s
// Configuration
pub const RAM_ORIGIN: usize = 0x2000_0000;
pub const RAM_LENGTH: usize = 16000 * 1024; // 16,000K
pub const STACK_SIZE_EXC: usize = 0x100; // 256 bytes per exception mode

// Stack Calculations
pub const STACK_TOP: usize = RAM_ORIGIN + RAM_LENGTH;
pub const STACK_END: usize = RAM_ORIGIN;
// all stacks fit in the 1MB, also the mmu only cares about the leading bits
pub const STACK_MMU: usize = 0x20F0_0000;
pub const STACK_TOP_FIQ: usize = STACK_TOP;
pub const STACK_TOP_IRQ: usize = STACK_TOP_FIQ - STACK_SIZE_EXC;
pub const STACK_TOP_ABORT: usize = STACK_TOP_IRQ - STACK_SIZE_EXC;
pub const STACK_TOP_UNDEFINED: usize = STACK_TOP_ABORT - STACK_SIZE_EXC;
pub const STACK_TOP_SUPERVISOR: usize = STACK_TOP_UNDEFINED - STACK_SIZE_EXC;

// Where the stack is physically located in RAM
pub const STACK_PHYS_BASE: usize = 0x20E0_0000;
// The Virtual "Ceiling" (where the SP starts)
pub const VIRTUAL_USER_STACK_TOP: usize = 0xC000_0000;
// The Virtual "Floor" (The start of the 1MB MMU section)
pub const VIRTUAL_USER_STACK_BASE: usize = 0xBFF0_0000;

pub const KERNEL_START: usize = 0x2000_0000;
pub const SECTION_SIZE: usize = 0x100_000; // how big a mmu entry maps to
pub const OFFSET_MASK: usize = SECTION_SIZE - 1; // 0x000F_FFFF

// just map the exception vectors, nothing else. they should not be over 1mb otherwise i am aa
pub const EXCEPTION_START: usize = 0x0;

pub const PERIPHERALS_START: usize = 0xF000_0000;
pub const PERIPHERALS_END: usize = 0xFFFF_FFFF;
