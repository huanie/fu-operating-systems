use crate::hardware::cpu;
use crate::thread::schedule::{CURRENT_THREAD, NUMBER_OF_THREADS, SCHEDULER};
use crate::util::busy_wait;
use core::convert::Infallible;
use core::ptr::{self, addr_of_mut, read_volatile, write_volatile};
use ringbuffer::{ConstGenericRingBuffer, RingBuffer};
use ufmt::uWrite;

const BUFFER_SIZE: usize = 32;
static mut READ_BUFFER: ConstGenericRingBuffer<char, BUFFER_SIZE> = ConstGenericRingBuffer::new();
static mut READ_QUEUE: ConstGenericRingBuffer<usize, NUMBER_OF_THREADS> =
    ConstGenericRingBuffer::new();

#[repr(C)]
pub struct Dbgu {
    cr: u32,   // 0x00
    mr: u32,   // 0x04
    ier: u32,  // 0x08
    idr: u32,  // 0x0c
    imr: u32,  // 0x10
    sr: u32,   // 0x14
    rhr: u32,  // 0x18
    thr: u32,  // 0x1c
    brgr: u32, // 0x20
}

pub const DBGU: *mut Dbgu = 0xfffff200 as *mut _;
const RXEN: u32 = 1 << 4;
const RSTRX: u32 = 1 << 2;
const TXEN: u32 = 1 << 6;
const RSTTX: u32 = 1 << 3;
const CHMOD: u32 = 0; // normal mode
const PAR: u32 = 1 << 11; // no parity
const TXRDY: u32 = 1 << 1;
const RXRDY: u32 = 1 << 0;

impl Dbgu {
    fn write_character(&mut self, c: char) {
        unsafe {
            while read_volatile(&self.sr) & TXRDY == 0 {}
            write_volatile(&mut self.thr, c as u32);
        }
    }
}
#[inline(never)]
extern "C" fn handler(c: usize) {
    let x = c as u8 as char;
    for _ in 0..10 {
        write(x);
        busy_wait(1_100_000);
    }
}

#[unsafe(export_name = "dbgu_interrupt")]
pub extern "C" fn interrupt() {
    unsafe {
        let dbgu = &DBGU;
        if read_volatile(&(**dbgu).sr) & RXRDY != 0 {
            let c = read_volatile(&(**dbgu).rhr) as usize;
            if let Some(thread_id) = (&mut *addr_of_mut!(READ_QUEUE)).dequeue() {
                let scheduler = &mut *addr_of_mut!(SCHEDULER);
                scheduler.wakeup(thread_id);
                let thread = scheduler.get_mut(thread_id);
                thread.r0 = c;
            } else {
                (&mut *addr_of_mut!(READ_BUFFER)).enqueue(c as u8 as char);
            }
        }
    }
}

pub fn write(c: char) {
    unsafe {
        let dbgu = &mut *DBGU;
        (*dbgu).write_character(c);
    }
}

/// this will set the current thread as blocked
pub fn read() -> char {
    if let Some(c) = unsafe { addr_of_mut!(READ_BUFFER).read_volatile().dequeue() } {
        c
    } else {
        unsafe {
            let scheduler = &mut (*addr_of_mut!(SCHEDULER));
            scheduler.block(
                crate::thread::thread_control_block::BlockReason::Read,
                &mut *addr_of_mut!(READ_QUEUE),
            );
            scheduler.change_next();
        };
        0 as char
    }
}

#[inline]
pub fn init() {
    unsafe {
        let dbgu = &mut *DBGU;
        write_volatile(&mut dbgu.mr, CHMOD | PAR);
        write_volatile(&mut dbgu.cr, RSTTX | RSTRX | RXEN | TXEN);
        write_volatile(&mut dbgu.ier, RXRDY);
    }
}

impl uWrite for Dbgu {
    type Error = Infallible;

    fn write_str(&mut self, s: &str) -> Result<(), Self::Error> {
        for c in s.chars() {
            self.write_character(c)
        }
        Ok(())
    }
}

#[macro_export]
macro_rules! print {
    ($($arg:expr),*) => (ufmt::uwrite!(unsafe { &mut *$crate::dbgu::DBGU }, $($arg,)*));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:expr),*) => (ufmt::uwriteln!(unsafe {&mut *$crate::dbgu::DBGU}, $($arg,)*));
}
