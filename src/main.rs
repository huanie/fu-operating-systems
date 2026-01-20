#![no_std]
#![no_main]

global_asm!(include_str!("./start.s"));

mod hardware;
mod syscall;
mod thread;
mod util;

use core::arch::global_asm;
use core::hint::spin_loop;
use core::panic::PanicInfo;
use hardware::*;

use crate::thread::schedule::{SCHEDULER, idle_thread};

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    if let Some(x) = info.message().as_str() {
        println!("Oh oh: {}", x);
        if let Some(x) = info.location() {
            println!("{}:{}", x.file(), x.line());
        }
        println!();
    }

    loop {}
}

extern "C" fn reader(_: usize) {
    loop {
        let c = syscall::read();
        if c.is_ascii_uppercase() {
            syscall::spawn(writer_active, c as usize);
        } else {
            syscall::spawn(writer_passive, c as usize);
        }
    }
}

extern "C" fn writer_active(c: usize) {
    let c = c as u8 as char;
    for _ in 0..10 {
        syscall::write(c);
        util::busy_wait(10000000);
    }
}

extern "C" fn writer_passive(c: usize) {
    let c = c as u8 as char;
    for _ in 0..10 {
        syscall::write(c);
        syscall::sleep(1000);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn kernel_main() -> ! {
    system_timer::init();
    dbgu::init();
    let scheduler = unsafe { &mut *{ &raw mut SCHEDULER } };
    scheduler.spawn(idle_thread, 0);
    scheduler.spawn(reader, 0);
    cpu::enable_interrupts();
    system_timer::set_interval::<{ hardware::system_timer::INTERVAL_MS }>();

    // the context switch will make it switch to the idle_thread
    loop {
        spin_loop();
    }
}
