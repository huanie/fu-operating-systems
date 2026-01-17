use crate::println;
use crate::thread::schedule::SCHEDULER;

const PITS: u32 = 1 << 0;
const BASE: u32 = 0xFFFFFD00;
const IER: u32 = BASE + 0x14;
const PIMR: u32 = BASE + 0x04; // Period Interval Mode Register
const SR: u32 = BASE + 0x10;

#[inline(always)]
pub fn init() {
    unsafe {
        (IER as *mut u32).write_volatile(PITS);
    }
}

pub fn set_interval<const MSEC: u32>() {
    const {
        assert!(
            MSEC < 2000,
            "The value is too big so it does not fit in the register"
        )
    }
    unsafe { (PIMR as *mut u32).write_volatile((32768 * MSEC) / 1000) }
}

#[unsafe(export_name = "system_timer_interrupt")]
extern "C" fn interrupt() {
    if unsafe { (SR as *mut u32).read_volatile() } & PITS != 0 {
        unsafe {
            (*core::ptr::addr_of_mut!(SCHEDULER)).change_next();
        }
    }
}
