#![no_std]
#![no_main]

global_asm!(include_str!("./start.s"));
global_asm!(include_str!("./cpu.s"));

use core::arch::global_asm;
use core::panic::PanicInfo;
mod dbgu;
mod exception;
mod hardware_register;
mod mutex;
mod system_timer;
mod thread;

/// This function is called on panic.
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("Oh oh");
    loop {}
}

#[allow(clippy::empty_loop)]
#[unsafe(no_mangle)]
pub extern "C" fn kernel_main() -> ! {
    dbgu::init();

    loop {
        dbgu::write(dbgu::read());
    }
}
