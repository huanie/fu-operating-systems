use crate::println;
use crate::thread::schedule::CURRENT_THREAD;
use crate::thread::thread_control_block::ThreadControlBlock;
use core::arch::naked_asm;
use core::hint::spin_loop;
use core::mem::offset_of;
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
extern "C" fn data_abort() {
    println!("Data abort");
    loop {
        spin_loop();
    }
}

#[unsafe(naked)]
extern "C" fn irq() {
    naked_asm!(
        // adjust the lr, the interrupt will set lr as the previous pc
        "sub lr, lr, #4",
        // save on the exception stack at first because r12 is my tcb pointer
        "push {{r12}}",
        // the current thread,
        "ldr r12, ={current_thread}",
        "ldr r12, [r12]",
        // save r0-r11
        "stmia r12!, {{r0-r11}}" ,
        // restore original r12 value (popped from first push)
        "pop {{r0}}",
        "stmia r12!, {{r0}}",
        // store sp and lr from user mode
        "stmia r12, {{sp, lr}}^",
        "nop",
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
        // restore from CURRENT_THREAD (which may have changed if we context switched)
        "ldr lr, ={current_thread}",
        "ldr lr, [lr]",
        // set cpsr
        "ldr r1, [lr, #{cpsr_offset}]",
        "msr spsr, r1",
        // restore the registers
        "ldmia lr, {{r0-r12, r13, r14}}^",
        "nop",
        // load pc
        "ldr lr, [lr, #{pc_offset}]",
        // switch back
        "movs pc, lr",
        current_thread = sym CURRENT_THREAD,
        cpsr_offset = const offset_of!(ThreadControlBlock, cpsr),
        pc_offset = const offset_of!(ThreadControlBlock, pc)
    )
}

#[unsafe(naked)]
/// r0 is the systemcall id
/// r1 is the argument if there is one
extern "C" fn software() {
    naked_asm!(
        // save on the exception stack at first because r12 is my tcb pointer
        "push {{r12}}",
        // the current thread,
        "ldr r12, ={current_thread}",
        "ldr r12, [r12]",
        // save r0-r11
        "stmia r12!, {{r0-r11}}" ,
        // restore original r12 value (popped from first push)
        "pop {{r0}}",
        "stmia r12!, {{r0}}",
        // store sp and lr from user mode
        "stmia r12, {{sp, lr}}^",
        "nop",
        // manual add 8 because writeback is not allowed with ^
        "add r12, r12, #8",
        // save pc (which is lr in exception mode)
        "stmia r12!, {{lr}}",
        // save cpsr
        "mrs r1, spsr",
        "stmia r12, {{r1}}",
        /* HANDLE INTERRUPT */

        // load the arguments
        "ldr r4, ={current_thread}",
        "ldr r4, [r4]",
        "ldr r0, [r4, #{r0_offset}]",
        "ldr r1, [r4, #{r1_offset}]",
        "ldr r2, [r4, #{r2_offset}]",
        "bl system_call",
        // put the return value into tcb
        "str r0, [r4, #{r0_offset}]",

        /* RESTORE CONTEXT */
        // restore from CURRENT_THREAD (which may have changed if we context switched)
        "ldr lr, ={current_thread}",
        "ldr lr, [lr]",
        // set cpsr
        "ldr r1, [lr, #{cpsr_offset}]",
        "msr spsr, r1",
        // restore the registers
        "ldmia lr, {{r0-r12, r13, r14}}^",
        "nop",
        // load pc
        "ldr lr, [lr, #{pc_offset}]",
        // switch back
        "movs pc, lr",
        current_thread = sym CURRENT_THREAD,
        cpsr_offset = const offset_of!(ThreadControlBlock, cpsr),
        pc_offset = const offset_of!(ThreadControlBlock, pc),
        r0_offset = const offset_of!(ThreadControlBlock, r0),
        r1_offset = const offset_of!(ThreadControlBlock, r1),
        r2_offset = const offset_of!(ThreadControlBlock, r2),
    )
}

#[unsafe(no_mangle)]
extern "C" fn undefined_instruction() {
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
    handler: extern "C" fn(),
) {
    let target = target as u32;
    let destination = *destination;
    unsafe {
        (target as *mut u32).write_volatile(encode_load(target, destination));
        (destination as *mut usize).write_volatile(handler as usize);
    }
}
