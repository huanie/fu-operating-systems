use crate::hardware_register::Register;
use crate::println;

const PITS: u32 = 1 << 0;
const BASE: u32 = 0xFFFFFD00;
const IER: *mut Register = (BASE + 0x14) as *mut _;
const PIMR: *mut Register = (BASE + 0x04) as *mut _; // Period Interval Mode Register
const SR: *mut Register = (BASE + 0x10) as *mut _;

#[inline(always)]
pub fn init() {
    unsafe {
        (*IER).write(PITS);
    }
}

pub fn set_interval<const MSEC: u32>() {
    const {
        assert!(
            MSEC < 2000,
            "The value is too big so it does not fit in the register"
        )
    }
    unsafe { (*PIMR).write((32768 * MSEC) / 1000) }
}

#[unsafe(export_name = "system_timer_interrupt")]
pub extern "C" fn interrupt() {
    if unsafe { (*SR).read() } & PITS != 0 {
        println!("!");
    }
}
