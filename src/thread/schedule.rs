use core::hint::spin_loop;
use core::ptr::addr_of_mut;

use crate::thread::thread_control_block::{State, ThreadControlBlock};

#[repr(C)]
pub struct Scheduler<const SIZE: usize> {
    current: usize,
    data: [ThreadControlBlock; SIZE],
}

extern "C" fn idle_thread() {
    loop {
        core::hint::spin_loop();
    }
}

#[inline(never)]
extern "C" fn start() -> ! {
    let scheduler = unsafe { &mut *addr_of_mut!(SCHEDULER) };
    let handler = scheduler.data[scheduler.current].handler;
    handler();
    end();
}

#[inline(never)]
extern "C" fn end() -> ! {
    let scheduler = unsafe { &mut *addr_of_mut!(SCHEDULER) };
    scheduler.data[scheduler.current].state = State::Done;
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
        }; SIZE];
        let mut i = 0;
        while i < SIZE {
            data[i].id = i;
            i += 1;
        }
        data[IDLE_THREAD_ID].state = State::Ready;
        Self {
            data,
            current: IDLE_THREAD_ID,
        }
    }

    const fn next(&self, current: usize) -> usize {
        (current + 1) % SIZE
    }

    pub fn change_next(&mut self) {
        let mut next = self.data[self.next(self.current)];
        while next.id != self.current {
            if next.state == State::Ready && next.id != IDLE_THREAD_ID {
                self.current = next.id;
                return;
            }
            next = self.data[self.next(next.id)];
        }
        self.current = IDLE_THREAD_ID;
    }

    pub fn spawn(&mut self, handler: extern "C" fn() -> ()) {
        let stack_head = core::ptr::addr_of!(__stack_top_user).addr();
        for (i, thread) in self.data.iter_mut().enumerate() {
            if thread.state == State::Done {
                // we use lr to jump back to the correct code
                thread.lr = start as *const () as usize;
                thread.handler = handler;
                thread.pc = start as *const () as usize;
                thread.sp = stack_head - i * STACK_SIZE;
                thread.cpsr = NEW_THREAD_CPSR;
            }
        }
    }
}

pub static mut SCHEDULER: Scheduler<NUMBER_OF_THREADS> = Scheduler::<16>::new();
