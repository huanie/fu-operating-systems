#![no_std]
#![no_main]
#![feature(adt_const_params)]
#![allow(incomplete_features)]

global_asm!(include_str!("./start.s"));

mod hardware;
#[allow(unused)]
mod syscall;
mod thread;
mod util;

use crate::thread::schedule::{SCHEDULER, idle_thread};
use core::arch::global_asm;
use core::hint::spin_loop;
use core::panic::PanicInfo;
use hardware::*;

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
    dbgu::init();
    mmu::init();
    system_timer::init();

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
