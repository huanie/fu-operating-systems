#pragma once
#include "util.hpp"
#include <stdint.h>
namespace aic {
using VoidFunction = auto (*)(void) -> void;
constexpr auto SYSIRQ = 1;

constexpr uint32_t BASE = 0xFFFFF000;
constexpr uint32_t SMR =
    BASE + 0x00; // Source Mode Register (32 sources, 4 bytes)
constexpr uint32_t SVR =
    BASE + 0x80; // Source Vector Register (32 sources, 4 bytes)
constexpr uint32_t IVR = BASE + 0x100;   // Interrupt Vector Register
constexpr uint32_t FVR = BASE + 0x104;   // Fast Interrupt Vector Register
constexpr uint32_t ISR = BASE + 0x108;   // Interrupt Status Register
constexpr uint32_t IPR = BASE + 0x10C;   // Interrupt Pending Register
constexpr uint32_t IMR = BASE + 0x110;   // Interrupt Mask Register
constexpr uint32_t CISR = BASE + 0x114;  // Core Interrupt Status Register
constexpr uint32_t IECR = BASE + 0x120;  // Interrupt Enable Command Register
constexpr uint32_t IDCR = BASE + 0x124;  // Interrupt Disable Command Register
constexpr uint32_t ICCR = BASE + 0x128;  // Interrupt Clear Command Register
constexpr uint32_t ISCR = BASE + 0x12C;  // Interrupt Set Command Register
constexpr uint32_t EOICR = BASE + 0x130; // End of Interrupt Command Register

template <uint32_t Index> auto inline _interrupt_source() {
  return SMR + Index * sizeof(uint32_t);
}

template <uint32_t Index> auto inline _interrupt_vector() {
  return SVR + Index * sizeof(uint32_t);
}

template <uint32_t Interrupt>
inline void enable_interrupt(VoidFunction handler) {
  static_assert(Interrupt <= 32, "There are only 32 interrupt lines");
  // install interrupt at the corresponding source and set line
  // level-sensitivity and priority to 0
  volatile_write(_interrupt_source<Interrupt>(), 0);
  volatile_write(_interrupt_vector<Interrupt>(),
                 reinterpret_cast<uint32_t>(handler));
  volatile_write(IECR, 1 << SYSIRQ);
}

inline void end() { volatile_write(EOICR, 666); }

inline void start() { volatile_read<uint32_t>(IVR); }
} // namespace aic
