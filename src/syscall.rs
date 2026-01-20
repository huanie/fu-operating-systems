use crate::hardware::{dbgu, system_timer};
use crate::thread::{CURRENT_THREAD, SCHEDULER, State};
use core::arch::asm;

#[repr(usize)]
pub enum Systemcall {
    ReadDbgu = 0,
    PrintDbgu,
    ThreadEnd,
    ThreadSpawn,
    ThreadSleep,
}
#[unsafe(no_mangle)]
extern "C" fn system_call(id: usize, arg: usize, arg1: usize) -> usize {
    let syscall = unsafe { core::mem::transmute::<usize, Systemcall>(id) };
    match syscall {
        Systemcall::ReadDbgu => {
            let c = dbgu::read();
            c as usize
        }
        Systemcall::PrintDbgu => {
            let c = arg as u8 as char;
            dbgu::write(c);
            0
        }
        Systemcall::ThreadEnd => {
            let current = unsafe { &mut **{ &raw mut CURRENT_THREAD } };
            current.state = State::Done;
            0
        }
        Systemcall::ThreadSpawn => {
            let fun: extern "C" fn(usize) -> () = unsafe { core::mem::transmute(arg) };
            let scheduler = unsafe { &mut *{ &raw mut SCHEDULER } };
            scheduler.spawn(fun, arg1);
            0
        }
        Systemcall::ThreadSleep => unsafe {
            system_timer::sleep(arg);
            0
        },
    }
}

pub fn sleep(ms: usize) {
    unsafe {
        asm!(
            "swi #0",
            in("r0") Systemcall::ThreadSleep as usize,
            in("r1") ms,
            clobber_abi("C"),
            options(nostack),
        );
    }
}

pub fn write(c: char) {
    unsafe {
        asm!(
            "swi #0",
            in("r0") Systemcall::PrintDbgu as usize,
            in("r1") c as usize,
            clobber_abi("C"),
            options(nostack),
        );
    }
}

pub fn spawn(handler: extern "C" fn(usize) -> (), arg: usize) {
    unsafe {
        asm!(
            "swi #0",
            in("r0") Systemcall::ThreadSpawn as usize,
            in("r1") handler as usize,
            in("r2") arg,
            clobber_abi("C"),
            options(nostack),
        );
    }
}

pub fn exit() {
    unsafe {
        asm!(
            "swi #0",
            in("r0") Systemcall::ThreadEnd as usize,
            clobber_abi("C"),
            options(nostack),
        );
    }
}

pub fn read() -> char {
    let result: usize;
    unsafe {
        asm!(
            "swi #0",
            inout("r0") Systemcall::ReadDbgu as usize => result,
            clobber_abi("C"),
            options(nostack),
        );
    }
    result as u8 as char
}
