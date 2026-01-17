use crate::println;
use crate::thread::thread_control_block::{State, ThreadControlBlock};
use core::hint::spin_loop;
use core::ptr::addr_of_mut;

pub struct Scheduler<const SIZE: usize> {
    data: [ThreadControlBlock; SIZE],
}

#[inline(never)]
pub extern "C" fn idle_thread(_: usize) {
    loop {
        spin_loop();
    }
}

#[inline(never)]
extern "C" fn start() {
    let current = unsafe { &*CURRENT_THREAD };
    let handler = current.handler;
    let arg = current.argument;

    // Use a volatile pointer to force the compiler to generate a real call
    unsafe {
        let func: extern "C" fn(usize) -> () = core::ptr::read_volatile(&handler);
        func(core::ptr::read_volatile(&arg));
    }

    end();
}

#[inline(never)]
extern "C" fn end() {
    println!("END");
    let current = unsafe { &mut *CURRENT_THREAD };
    current.state = State::Done;
    loop {
        spin_loop();
    }
}

const IDLE_THREAD_ID: usize = 0;
const STACK_SIZE: usize = 512;
const NUMBER_OF_THREADS: usize = 16;
const NEW_THREAD_CPSR: usize = 0x10;

unsafe extern "C" {
    static __stack_top_user: usize;
}

impl<const SIZE: usize> Scheduler<SIZE> {
    pub const fn new() -> Self {
        let mut data = [ThreadControlBlock {
            r0: 0,
            r1: 0,
            r2: 0,
            r3: 0,
            r4: 0,
            r5: 0,
            r6: 0,
            r7: 0,
            r8: 0,
            r9: 0,
            r10: 0,
            r11: 0,
            r12: 0,
            sp: 0,
            lr: 0,
            pc: 0,
            cpsr: NEW_THREAD_CPSR,
            handler: idle_thread,
            state: State::Done,
            id: 0,
            argument: 0,
        }; SIZE];
        let mut i = 0;
        while i < SIZE {
            data[i].id = i;
            i += 1;
        }

        Self { data }
    }

    const fn next(current: usize) -> usize {
        (current + 1) % SIZE
    }

    pub fn change_next(&mut self) {
        let current = unsafe { &*CURRENT_THREAD }.id;
        let mut next = &self.data[Self::next(current)];
        while next.id != current {
            if next.state == State::Ready && next.id != IDLE_THREAD_ID {
                unsafe { CURRENT_THREAD = addr_of_mut!(self.data[next.id]) }
                return;
            }
            next = &self.data[Self::next(next.id)];
        }

        unsafe { CURRENT_THREAD = addr_of_mut!(self.data[IDLE_THREAD_ID]) }
    }

    pub fn spawn(&mut self, handler: extern "C" fn(usize) -> (), arg: usize) {
        let stack_head = unsafe { &__stack_top_user as *const usize as usize };
        for (i, thread) in self.data.iter_mut().enumerate() {
            if thread.state == State::Done {
                // we use lr to jump back to the correct code
                thread.lr = start as *const () as usize;
                thread.handler = handler;
                thread.pc = start as *const () as usize;
                thread.sp = stack_head - i * STACK_SIZE;
                thread.cpsr = NEW_THREAD_CPSR;
                thread.argument = arg;
                thread.state = State::Ready;
                return;
            }
        }
    }
}

pub static mut SCHEDULER: Scheduler<NUMBER_OF_THREADS> = Scheduler::<NUMBER_OF_THREADS>::new();
pub static mut CURRENT_THREAD: *mut ThreadControlBlock =
    unsafe { addr_of_mut!(SCHEDULER.data[IDLE_THREAD_ID]) };
