use crate::thread::schedule::SCHEDULER;
use crate::thread::{BlockReason, CURRENT_THREAD, NUMBER_OF_THREADS, State, ThreadControlBlock};

pub const INTERVAL_MS: usize = 50;
const PITS: usize = 1 << 0;
const BASE: usize = 0xFFFFFD00;
const IER: usize = BASE + 0x14;
const PIMR: usize = BASE + 0x04; // Period Interval Mode Register
const SR: usize = BASE + 0x10;

#[inline(always)]
pub fn init() {
    unsafe {
        (IER as *mut usize).write_volatile(PITS);
    }
}

pub fn set_interval<const MSEC: usize>() {
    let ticks: usize = const {
        match (32768_usize).checked_mul(MSEC) {
            Some(val) => val / 1000,
            None => panic!("Overflow occurred during tick calculation!"),
        }
    };
    unsafe { (PIMR as *mut usize).write_volatile(ticks) }
}

#[unsafe(export_name = "system_timer_interrupt")]
extern "C" fn interrupt() {
    if unsafe { (SR as *mut usize).read_volatile() } & PITS != 0 {
        unsafe {
            let mut mask = SLEEPERS_MASK;
            while mask != 0 {
                // 0101
                // index 0 first
                let thread_id = mask.trailing_zeros() as usize;
                SLEEPERS[thread_id] = SLEEPERS[thread_id].saturating_sub(INTERVAL_MS);
                if SLEEPERS[thread_id] == 0 {
                    // remove that thread from the mask
                    SLEEPERS_MASK &= !(1 << thread_id);
                    (&mut *{ &raw mut SCHEDULER }).wakeup(thread_id);
                }
                // done checking that thread, clear it to continue iteration
                mask &= !(1 << thread_id);
            }
            (*{ &raw mut SCHEDULER }).change_next();
        }
    }
}

static mut SLEEPERS: [usize; NUMBER_OF_THREADS] = [0; NUMBER_OF_THREADS];
static mut SLEEPERS_MASK: usize = 0;

pub fn sleep(ms: usize) {
    let scheduler = unsafe { &mut *{ &raw mut SCHEDULER } };
    scheduler.block(BlockReason::Sleep);
    let id = unsafe { &*CURRENT_THREAD }.id;
    unsafe {
        SLEEPERS[id] = ms;
        SLEEPERS_MASK |= 1 << id;
    }
    scheduler.change_next();
}
