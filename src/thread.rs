#[repr(u8)]
pub enum State {
    Done,
    Running,
    Blocked,
    Ready,
}
#[repr(C)]
pub struct ThreadControlBlock {
    // DO NOT REORDER
    pub r0: u32,
    pub r1: u32,
    pub r2: u32,
    pub r3: u32,
    pub r4: u32,
    pub r5: u32,
    pub r6: u32,
    pub r7: u32,
    pub r8: u32,
    pub r9: u32,
    pub r10: u32,
    pub r11: u32,
    pub r12: u32,
    pub sp: u32,   // User mode SP (r13)
    pub lr: u32,   // User mode LR (r14)
    pub pc: u32,   // Return address (adjusted IRQ LR)
    pub cpsr: u32, // Saved CPSR (SPSR in IRQ mode)

    pub handler: extern "C" fn() -> (),
    pub state: State,
    pub id: u8,
}
