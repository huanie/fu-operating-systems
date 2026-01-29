use super::memlayout::*;
use core::arch::asm;
use core::marker::ConstParamTy;

#[derive(PartialEq, Eq, ConstParamTy)]
#[repr(u8)]
pub enum AccessControl {
    Full,
    OnlyUserRead,
    KernelReadOnly,
}

impl AccessControl {
    pub const fn ap(&self) -> u8 {
        match self {
            AccessControl::Full => 0b11,
            AccessControl::OnlyUserRead => 0b10,
            AccessControl::KernelReadOnly => 0b00,
        }
    }
    pub const fn domain(&self) -> u8 {
        match self {
            AccessControl::Full => 0b10,
            AccessControl::OnlyUserRead => 0b10,
            AccessControl::KernelReadOnly => 0b10,
        }
    }
}

const MMU_TABLE_ALIGN: usize = 2usize.pow(14);
#[repr(C, align(16384))] // 2^14, only the leading bits are read. the others will be ignored!
pub struct MmuTable([usize; 4096]);
const _: () = assert!(core::mem::align_of::<MmuTable>() == MMU_TABLE_ALIGN);
const fn sections(start: usize, end: usize) -> usize {
    if end <= start {
        panic!("incorrect usage");
    }
    // Calculate the total number of bytes in the range
    let length = end - start + 1;
    length.div_ceil(SECTION_SIZE)
}

impl MmuTable {
    pub const fn new() -> Self {
        // Initializing with 0 creates "Fault" entries (type 00)
        // Satisfies "Kernel only access to occupied areas"
        MmuTable([0; 4096])
    }
    pub const fn init() -> Self {
        let mut table = Self::new();
        // kernel code
        table.map_section::<KERNEL_START, 1, KERNEL_START, { AccessControl::Full }>(); // TODO: just as a test
        // exception vectors
        table.map_section::<EXCEPTION_START, 1, EXCEPTION_START, { AccessControl::Full }>();
        // the stacks
        table.map_section::<STACK_MMU, 1, STACK_MMU, { AccessControl::Full }>();
        // the peripherals
        table.map_section::<PERIPHERALS_START, {sections(PERIPHERALS_START, PERIPHERALS_END)}, PERIPHERALS_START, { AccessControl::Full }>();

        //table.map_section::<0, 4096, 0, { AccessControl::Full }>();
        table
    }
    const fn map_section<
        const PHYSICAL: usize, // the physical address
        const SECTIONS: usize, // how many sections are needed (1mb) to cover the range
        const VIRTUAL: usize,  // where to map the physical address to
        const ACCESS_CONTROLL: AccessControl,
    >(
        &mut self,
    ) {
        struct Assert<const X: usize> {}
        impl<const X: usize> Assert<X> {
            const OK: bool = {
                assert!(X & OFFSET_MASK == 0);
                true
            };
        }
        let _ = Assert::<PHYSICAL>::OK;
        let _ = Assert::<VIRTUAL>::OK;

        let index = VIRTUAL >> 20;

        // Build the descriptor
        let mut i = 0;
        while i < SECTIONS {
            self.0[index + i] = (PHYSICAL + (SECTION_SIZE * i))
            | ((ACCESS_CONTROLL.ap() as usize) << 10)
            | ((ACCESS_CONTROLL.domain() as usize) << 5)
            | (1 << 4) // this just needs to be 1
                | 0b10 // section table
;
            i += 1;
        }
    }
}
static TABLE: MmuTable = MmuTable::init();
#[inline]
pub fn init() {
    const DOMAIN_ACCESS: usize = 0x55555555; // 0101010...
    let table_ptr = &TABLE as *const _ as usize;
    unsafe {
        core::arch::asm!(
            // 1. Set Translation Table Base
            "mcr p15, 0, {table_ptr}, c2, c0, 0",

            // 2. Set Domain Access (Client for all domains)
            "mcr p15, 0, {domains}, c3, c0, 0",

            // 3. Invalidate TLB and Caches (SBZ - Should Be Zero)
            "mov {tmp}, #0",
            "mcr p15, 0, {tmp}, c7, c7, 0",  // Invalidate Caches
            "mcr p15, 0, {tmp}, c8, c7, 0",  // Invalidate TLB

            // 4. Data Synchronization Barrier (Drain Write Buffer)
            "mcr p15, 0, {tmp}, c7, c10, 4",

            // 5. Read-Modify-Write Control Register
            "mrc p15, 0, {tmp}, c1, c0, 0",
            "orr {tmp}, {tmp}, #1",          // Set bit 0 (MMU Enable)
            "mcr p15, 0, {tmp}, c1, c0, 0",

            // 6. Pipeline Flush / ISB
            "nop",
            "nop",
            "nop",
            table_ptr = in(reg) table_ptr,
            domains = in(reg) DOMAIN_ACCESS,
            tmp = out(reg) _, // Let the compiler pick any scratch register
        )
    }
}
