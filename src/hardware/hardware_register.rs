#[repr(transparent)]
pub struct Register(u32);
impl Register {
    #[inline(always)]
    pub fn read(&self) -> u32 {
        unsafe { core::ptr::read_volatile(&self.0) }
    }
    #[inline(always)]
    pub fn write(&mut self, data: u32) {
        unsafe { core::ptr::write(&mut self.0, data) }
    }
}
