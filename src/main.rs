#![no_std]
#![no_main]

global_asm!(include_str!("./start.s"));

mod hardware;
mod thread;

use core::arch::global_asm;
use core::panic::PanicInfo;
use hardware::*;

/// This function is called on panic.
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("Oh oh");
    loop {}
}

#[allow(clippy::empty_loop)]
#[unsafe(no_mangle)]
pub extern "C" fn kernel_main() -> ! {
    println!("Hello World");

    cpu::enable_interrupts();
    dbgu::init();
    system_timer::init();
    system_timer::set_interval::<50>();
    loop {
        dbgu::write(dbgu::read());
    }
}
