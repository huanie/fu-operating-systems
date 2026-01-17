const BASE: u32 = 0xFFFFF000;
const SMR: u32 = BASE; // Source Mode Register (32 sources, 4 bytes)
const IECR: u32 = BASE + 0x120; // Interrupt Enable Command Register

const SYSIRQ: u32 = 1;

#[inline(always)]
pub fn init() {
    unsafe {
        ((SMR + SYSIRQ * (size_of::<u32>() as u32)) as *mut u32).write_volatile(0);
        (IECR as *mut u32).write_volatile(1 << SYSIRQ);
    }
}
