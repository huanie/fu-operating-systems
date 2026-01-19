use core::arch::asm;

use crate::hardware::dbgu;

#[repr(usize)]
pub enum Systemcall {
    ReadDbgu = 0,
    PrintDbgu,
    ThreadEnd,
    ThreadSpawn,
    ThreadSleep,
}
#[unsafe(no_mangle)]
extern "C" fn system_call(id: usize, arg: usize) -> usize {
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
        Systemcall::ThreadEnd => todo!(),
        Systemcall::ThreadSpawn => todo!(),
        Systemcall::ThreadSleep => todo!(),
    }
}

pub fn write(c: char) {
    unsafe {
        asm!(
            "swi #0",
            in("r0") Systemcall::PrintDbgu as usize,
            in("r1") c as u32,
            lateout("r0") _, // r0 is clobbered by the syscall return value
            lateout("r1") _, // r1 might be clobbered too
        );
    }
}

pub fn read() -> char {
    let result: usize;
    unsafe {
        asm!(
            "swi #0",
            inout("r0") Systemcall::ReadDbgu as usize => result,
        );
    }
    result as u8 as char
}
