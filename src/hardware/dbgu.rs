use crate::println;
use crate::thread::mutex::Mutex;
use crate::thread::schedule::SCHEDULER;
use crate::util::busy_wait;
use core::convert::Infallible;
use core::mem::MaybeUninit;
use core::ptr::{self, addr_of_mut, read_volatile, write_volatile};
use ufmt::uWrite;

const BUFFER_LENGTH: usize = 32;
static mut BUFFER: [MaybeUninit<char>; BUFFER_LENGTH] = [MaybeUninit::uninit(); BUFFER_LENGTH];
static mut BUFFER_SIZE: usize = 0;

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

//pub const DBGU: *mut Dbgu = ;
pub static DBGU: Mutex<*mut Dbgu> = Mutex::new(0xfffff200 as *mut _);
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
        busy_wait(1_000_000);
    }
}

#[unsafe(export_name = "dbgu_interrupt")]
pub extern "C" fn interrupt() {
    unsafe {
        let dbgu = DBGU.lock();
        if read_volatile(&(**dbgu).sr) & RXRDY != 0 {
            let c = read_volatile(&(**dbgu).rhr) as usize;
            drop(dbgu);
            (*addr_of_mut!(SCHEDULER)).spawn(handler, c);
        }
    }
}

pub fn write(c: char) {
    unsafe {
        let dbgu = *DBGU.lock();
        (*dbgu).write_character(c);
    }
}

#[inline(always)]
pub fn read() -> char {
    pop_buffer()
}

fn push_buffer(c: char) {
    unsafe {
        let index = ptr::read_volatile(&raw const BUFFER_SIZE);
        // return if the buffer is full
        if index >= BUFFER_LENGTH {
            return;
        }

        BUFFER[index].write(c);

        ptr::write_volatile(&raw mut BUFFER_SIZE, index + 1);
    }
}

fn pop_buffer() -> char {
    unsafe {
        loop {
            let index = ptr::read_volatile(&raw const BUFFER_SIZE);
            if index > 0 {
                break;
            }
        }

        let index = ptr::read_volatile(&raw const BUFFER_SIZE);

        let c = BUFFER[index - 1].assume_init();
        ptr::write_volatile(&raw mut BUFFER_SIZE, index - 1);

        c
    }
}

#[inline]
pub fn init() {
    unsafe {
        let dbgu = &mut **DBGU.lock();
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
    ($($arg:expr),*) => (ufmt::uwrite!(unsafe { &mut **$crate::dbgu::DBGU.lock() }, $($arg,)*));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:expr),*) => (ufmt::uwriteln!(unsafe {&mut **$crate::dbgu::DBGU.lock() }, $($arg,)*));
}
