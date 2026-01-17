#![no_std]
#![no_main]

global_asm!(include_str!("./start.s"));

mod hardware;
mod thread;
mod util;

use core::arch::global_asm;
use core::hint::spin_loop;
use core::panic::PanicInfo;
use hardware::*;

use crate::thread::schedule::{SCHEDULER, idle_thread};

/// This function is called on panic.
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("Oh oh");
    loop {}
}

#[allow(clippy::empty_loop)]
#[unsafe(no_mangle)]
pub extern "C" fn kernel_main() -> ! {
    system_timer::init();
    dbgu::init();
    unsafe {
        (*core::ptr::addr_of_mut!(SCHEDULER)).spawn(idle_thread, 0);
    }

    println!("Hello World");
    cpu::enable_interrupts();
    system_timer::set_interval::<50>();
    loop {
        spin_loop();
    }
}
