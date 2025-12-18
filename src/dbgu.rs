use crate::hardware_register::Register;
use core::mem::MaybeUninit;

const BUFFER_SIZE: usize = 32;
static mut BUFFER: [MaybeUninit<u32>; BUFFER_SIZE] = [MaybeUninit::uninit(); BUFFER_SIZE];
static mut BUFFER_INDEX: usize = 0;

#[repr(C)]
struct Dbgu {
    cr: Register,   // 0x00
    mr: Register,   // 0x04
    ier: Register,  // 0x08
    idr: Register,  // 0x0c
    imr: Register,  // 0x10
    sr: Register,   // 0x14
    rhr: Register,  // 0x18
    thr: Register,  // 0x1c
    brgr: Register, // 0x20
}

const DBGU: *mut Dbgu = 0xfffff200 as *mut _;

const RXEN: u32 = 1 << 4;
const RXDIS: u32 = 1 << 5;
const RSTRX: u32 = 1 << 2;
const TXEN: u32 = 1 << 6;
const TXDIS: u32 = 1 << 7;
const RSTTX: u32 = 1 << 3;
const CHMOD: u32 = 0; // normal mode
const PAR: u32 = 1 << 11; // no parity
const TXRDY: u32 = 1 << 1;
const FRAME: u32 = 1 << 6;
const RSTSTA: u32 = 1 << 8;
const OVRE: u32 = 1 << 5;
const RXRDY: u32 = 1 << 0;

impl Dbgu {
    fn write_character(&mut self, c: char) {
        while self.sr.read() & TXRDY == 0 {}
        self.thr.write(c as u32);
    }
}

pub fn read() -> char {
    todo!()
}

fn push_buffer(c: char) {
    unsafe {
        if BUFFER_INDEX >= BUFFER_SIZE {
            return;
        }
        BUFFER[BUFFER_INDEX].write(c as u32);
    }
}

#[inline]
pub fn init() {
    unsafe {
        (*DBGU).mr.write(CHMOD | PAR);
        (*DBGU).cr.write(RSTTX | RSTRX | RXEN | TXEN);
        (*DBGU).ier.write(RXRDY);
    }
}

use core::fmt;
impl fmt::Write for Dbgu {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.chars() {
            unsafe { (*DBGU).write_character(c) }
        }
        Ok(())
    }
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::dbgu::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)));
}

#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    unsafe {
        (*DBGU).write_fmt(args).unwrap_unchecked();
    }
}
