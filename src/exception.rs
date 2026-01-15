use core::ops::Deref;

#[repr(u32)]
enum Exception {
    DataAbort = 0x10,
    Software = 0x8,
    Irq = 0x18,
    Undefined = 0x4,
}

struct Trampoline(u32);
struct TrampolineIndex(u32);

impl Deref for TrampolineIndex {
    type Target = u32;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl Trampoline {
    fn get(&self, index: u32) -> TrampolineIndex {
        TrampolineIndex(self.0 + index * (core::mem::size_of::<u32>() as u32))
    }
}

mod private {
    use super::*;
    use crate::println;

    const TRAMPOLINE: Trampoline = Trampoline(0x30);

    #[unsafe(no_mangle)]
    pub extern "C" fn init_exceptions() {
        install_exception_handler(Exception::DataAbort, TRAMPOLINE.get(0), data_abort);
        install_exception_handler(Exception::Software, TRAMPOLINE.get(1), software);
        install_exception_handler(
            Exception::Undefined,
            TRAMPOLINE.get(2),
            undefined_instruction,
        );
        install_exception_handler(Exception::Irq, TRAMPOLINE.get(3), irq);
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn data_abort() {
        println!("Data abort");
        loop {}
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn irq() {
        println!("irq");
        crate::dbgu::interrupt();
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn software() {
        println!("software");
        loop {}
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn undefined_instruction() {
        println!("Undefined instruction");
        loop {}
    }

    const fn encode_load(target: u32, destination: u32) -> u32 {
        let pc = target + 8;
        let offset = destination - pc;

        if offset >= 0xFFF {
            panic!("Destination is out of bounds");
        }

        0xE59FF000 | offset
    }

    fn install_exception_handler(
        target: Exception,
        destination: TrampolineIndex,
        handler: extern "C" fn(),
    ) {
        let target = target as u32;
        let destination = *destination;
        unsafe {
            (target as *mut u32).write_volatile(encode_load(target, destination));
            (destination as *mut usize).write_volatile(handler as usize);
        }
    }
}
