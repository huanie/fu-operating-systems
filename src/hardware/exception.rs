use crate::println;
use crate::thread::schedule::SCHEDULER;
use crate::thread::thread_control_block::ThreadControlBlock;
use core::arch::naked_asm;
use core::hint::spin_loop;
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

const TRAMPOLINE: Trampoline = Trampoline(0x30);

#[unsafe(no_mangle)]
extern "C" fn init_exceptions() {
    install_exception_handler(Exception::DataAbort, TRAMPOLINE.get(0), data_abort);
    install_exception_handler(Exception::Software, TRAMPOLINE.get(1), software);
    install_exception_handler(
        Exception::Undefined,
        TRAMPOLINE.get(2),
        undefined_instruction,
    );
    install_exception_handler(Exception::Irq, TRAMPOLINE.get(3), irq);
    crate::aic::init();
}

#[unsafe(no_mangle)]
extern "C" fn data_abort() -> ! {
    println!("Data abort");
    loop {
        spin_loop();
    }
}

#[unsafe(naked)]
extern "C" fn irq() -> ! {
    naked_asm!(
        // adjust the lr, the interrupt will set lr as the previous pc
        "sub lr, lr, #4",
        // save on the exception stack at first because r12 is my tcb pointer
        "push {{r12}}",
        // the current thread,
        "ldr r12, ={scheduler}",
        "ldr r12, [r12]",
        // save r0-r12
        "stmia r12!, {{r0-r11}}" ,
        // r12 is now in r0
        "pop {{r0}}",
        "stmia r12!, {{r0}}",
        // store sp and lr from user mode
        "stmia r12, {{sp, lr}}^",
        // manual add 8 because writeback is not allowed with ^
        "add r12, r12, #8",
        // save pc (which is lr in exception mode)
        "stmia r12!, {{lr}}",
        // save cpsr
        "mrs r1, spsr",
        "stmia r12, {{r1}}",
        /* HANDLE INTERRUPT */

        "bl system_timer_interrupt",
        "bl dbgu_interrupt",

        /* RESTORE CONTEXT */
        "ldr r0, ={scheduler}",
        "ldr r0, [r0]",
        // set cpsr
        "ldr r1, [r0, #{cpsr_offset}]",
        "msr spsr, r1",
        // switch back
        "ldmia r0, {{r0-r12, r13, r14, r15}}^",
        scheduler = sym SCHEDULER,
        cpsr_offset = const core::mem::offset_of!(ThreadControlBlock, cpsr)
    )
}

#[unsafe(no_mangle)]
extern "C" fn software() -> ! {
    println!("software");
    loop {
        spin_loop();
    }
}

#[unsafe(no_mangle)]
extern "C" fn undefined_instruction() -> ! {
    println!("Undefined instruction");
    loop {
        spin_loop();
    }
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
    handler: extern "C" fn() -> !,
) {
    let target = target as u32;
    let destination = *destination;
    unsafe {
        (target as *mut u32).write_volatile(encode_load(target, destination));
        (destination as *mut usize).write_volatile(handler as usize);
    }
}
