#pragma once

namespace cpu {
enum class Mode {
  usr = 0x10,
  fiq = 0x11,
  irq = 0x12,
  svc = 0x13,
  abt = 0x17,
  und = 0x1b,
  sys = 0xf
};
extern "C" void disable_irq();
extern "C" void enable_irq();
extern "C" bool irq_disabled();
} // namespace cpu
