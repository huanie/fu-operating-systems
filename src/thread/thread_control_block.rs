#[repr(u8)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum State {
    Done,
    Blocked,
    Ready,
}
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct ThreadControlBlock {
    // DO NOT REORDER
    pub r0: usize,
    pub r1: usize,
    pub r2: usize,
    pub r3: usize,
    pub r4: usize,
    pub r5: usize,
    pub r6: usize,
    pub r7: usize,
    pub r8: usize,
    pub r9: usize,
    pub r10: usize,
    pub r11: usize,
    pub r12: usize,
    pub sp: usize,   // User mode SP (r13)
    pub lr: usize,   // User mode LR (r14)
    pub pc: usize,   // User mode PC (r15)
    pub cpsr: usize, // Saved CPSR (SPSR in IRQ mode)

    pub handler: extern "C" fn(usize) -> (),
    pub state: State,
    pub id: usize,
    pub argument: usize,
}
