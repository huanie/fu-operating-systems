#pragma once

#include "util.hpp"
#include <stdint.h>

namespace system_timer {
// AT91RM9200 System Timer base addr
constexpr uint32_t BASE = 0xFFFFFD00;

// offset
constexpr uint32_t CR = BASE + 0x00;    // Control Register
constexpr uint32_t PIMR = BASE + 0x04;  // Period Interval Mode Register
constexpr uint32_t WDTMR = BASE + 0x08; // Watchdog Mode Register
constexpr uint32_t RTMR = BASE + 0x0C;  // Real-time Mode Register
constexpr uint32_t SR = BASE + 0x10;    // Status Register
constexpr uint32_t IER = BASE + 0x14;   // Interrupt Enable Register
constexpr uint32_t IDR = BASE + 0x18;   // Interrupt Disable Register
constexpr uint32_t IMR = BASE + 0x1C;   // Interrupt Mask Register

// control
constexpr uint32_t PITS = 1 << 0; // Period Interval Timer Status

extern "C" void system_timer_interrupt();

inline void init() {
  // enable periodic timer
  volatile_write(IER, PITS);
}

inline void set_interval(uint32_t msec) {
  if (msec >= 2000) {
    volatile_write(PIMR, 0);
  } else {
    volatile_write(PIMR, (32768 * msec) / 1000);
  }
}

} // namespace system_timer
