#[inline(never)]
pub fn busy_wait(usec: usize) {
    let loops = usec * 45;
    unsafe {
        core::arch::asm!(
            "2:",
            "subs {0}, {0}, #1",
            "bne 2b",
            inout(reg) loops => _,
        );
    }
}
