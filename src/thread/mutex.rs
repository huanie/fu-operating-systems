use core::cell::UnsafeCell;
use core::ops::{Deref, DerefMut};
pub struct Mutex<T> {
    locked: UnsafeCell<bool>,
    inner: UnsafeCell<T>,
}

pub struct MutexGuard<'a, T> {
    mutex: &'a Mutex<T>,
}

impl<T> Mutex<T> {
    pub fn new(o: T) -> Self {
        Self {
            locked: UnsafeCell::new(false),
            inner: UnsafeCell::new(o),
        }
    }
    pub fn lock(&'_ self) -> MutexGuard<'_, T> {
        // disable interrupts
        unsafe {
            core::arch::asm!(
                "mrs {tmp}, cpsr",
                "orr {tmp}, {tmp}, #(1 << 7)",
                "msr cpsr_c, {tmp}",
                tmp = out(reg) _,
            );
            *self.locked.get() = true;
        }
        MutexGuard { mutex: self }
    }
}

impl<T> Drop for MutexGuard<'_, T> {
    fn drop(&mut self) {
        unsafe {
            core::arch::asm!(
                "mrs {tmp}, cpsr",
                "bic {tmp}, {tmp}, #(1 << 7)",
                "msr cpsr_c, {tmp}",
                tmp = out(reg) _,
            )
        }
        unsafe { *self.mutex.locked.get() = false }
    }
}

impl<T> Deref for MutexGuard<'_, T> {
    type Target = T;

    fn deref(&self) -> &T {
        unsafe { &*self.mutex.inner.get() }
    }
}

impl<T> DerefMut for MutexGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.mutex.inner.get() }
    }
}

unsafe impl<T> Sync for Mutex<T> {}
