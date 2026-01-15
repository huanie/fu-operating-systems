use crate::hardware_register::Register;
use core::convert::Infallible;
use core::mem::MaybeUninit;
use core::ptr;
use ufmt::uWrite;

const BUFFER_LENGTH: usize = 32;
static mut BUFFER: [MaybeUninit<char>; BUFFER_LENGTH] = [MaybeUninit::uninit(); BUFFER_LENGTH];
static mut BUFFER_SIZE: usize = 0;

#[repr(C)]
pub struct Dbgu {
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
        while self.sr.read() & TXRDY == 0 {}
        self.thr.write(c as u32);
    }
}

#[unsafe(export_name = "dbgu_interrupt")]
pub extern "C" fn interrupt() {
    unsafe {
        if (*DBGU).sr.read() & RXRDY != 0 {
            push_buffer((*DBGU).rhr.read() as u8 as char);
        }
    }
}

pub fn write(c: char) {
    unsafe {
        (*DBGU).write_character(c);
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
        (*DBGU).mr.write(CHMOD | PAR);
        (*DBGU).cr.write(RSTTX | RSTRX | RXEN | TXEN);
        (*DBGU).ier.write(RXRDY);
    }
}

impl uWrite for Dbgu {
    type Error = Infallible;

    fn write_str(&mut self, s: &str) -> Result<(), Self::Error> {
        for c in s.chars() {
            unsafe { (*DBGU).write_character(c) }
        }
        Ok(())
    }
}

#[macro_export]
macro_rules! print {
    ($($arg:expr)*) => (ufmt::uwrite!(unsafe {&mut *$crate::dbgu::DBGU}, $($arg,)*));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:expr),*) => (ufmt::uwriteln!(unsafe {&mut *$crate::dbgu::DBGU}, $($arg,)*));
}
