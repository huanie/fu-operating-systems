#[inline(always)]
pub fn enable_interrupts() {
    unsafe {
        core::arch::asm!(
            "mrs {tmp}, cpsr",
            "bic {tmp}, {tmp}, #0x80",
            "msr cpsr_c, {tmp}",
            tmp = out(reg) _,
            options(nomem, nostack)
        );
    }
}

#[inline(always)]
pub fn disable_interrupts() {
    unsafe {
        core::arch::asm!(
            "mrs {tmp}, cpsr",
            "orr {tmp}, {tmp}, #0x80",
            "msr cpsr_c, {tmp}",
            tmp = out(reg) _,
            options(nomem, nostack)
        );
    }
}

#[inline(always)]
pub fn interrupts_disabled() -> bool {
    let cpsr: u32;
    unsafe {
        core::arch::asm!(
            "mrs {}, cpsr",
            out(reg) cpsr,
            options(nomem, nostack, preserves_flags)
        );
    }
    (cpsr & 0x80) != 0
}
