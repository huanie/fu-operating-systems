#pragma once
#include "util.hpp"
#include <stdint.h>

namespace system_timer {
  // AT91RM9200 System Timer base addr
  constexpr uint32_t BASE = 0xFFFFFD00;
  
  // offset
  constexpr uint32_t CR = BASE + 0x00;   // Control Register
  constexpr uint32_t PIMR = BASE + 0x04;  // Period Interval Mode Register
  constexpr uint32_t WDTMR = BASE + 0x08; // Watchdog Mode Register
  constexpr uint32_t RTMR = BASE + 0x0C;   // Real-time Mode Register
  constexpr uint32_t SR = BASE + 0x10;     // Status Register
  constexpr uint32_t IER = BASE + 0x14;    // Interrupt Enable Register
  constexpr uint32_t IDR = BASE + 0x18;    // Interrupt Disable Register
  constexpr uint32_t IMR = BASE + 0x1C;    // Interrupt Mask Register
  
  // control
  constexpr uint32_t PITEN = 1 << 24;      // Period Interval Timer Enable
  constexpr uint32_t PITIEN = 1 << 25;     // Period Interval Timer Interrupt Enable
  constexpr uint32_t PITS = 1 << 0;        // Period Interval Timer Status
  
  constexpr uint32_t INTERRUPT_INTERVAL_MS = 100;  // 100 ms period
  constexpr uint32_t ST_SLOW_CLOCK_HZ = 32768;
  constexpr uint32_t RAW_PIMR_VALUE =
      static_cast<uint32_t>((static_cast<uint64_t>(ST_SLOW_CLOCK_HZ) *
                            INTERRUPT_INTERVAL_MS) / 1000);
  constexpr uint32_t PIMR_VALUE =
      RAW_PIMR_VALUE == 0 ? 1 : (RAW_PIMR_VALUE & 0xFFFFF);
  
  inline void init() {
    // 1. Disable timer and its interrupt first
    volatile_write(CR, 0);
    volatile_write(IDR, PITIEN);  // Disable interrupt
    
    // 2. Clear pending status by reading SR
    volatile_read<uint32_t>(SR);
    
    // 3. Program period while disabled
    volatile_write(PIMR, PIMR_VALUE);
    
    // 4. Clear status again for safety
    volatile_read<uint32_t>(SR);
    
    // 5. Enable interrupt before starting
    volatile_write(IER, PITIEN);
    
    // 6. Clear status again
    volatile_read<uint32_t>(SR);
    
    // 7. Start timer so first interrupt fires after PIMR_VALUE cycles
    volatile_write(CR, PITEN | PITIEN);
    
    // 8. Final status clear to avoid spurious IRQ
    volatile_read<uint32_t>(SR);
  }
  
  inline bool is_interrupt_pending() {
    return (volatile_read<uint32_t>(SR) & PITS) != 0;
  }
  
  inline void clear_interrupt() {
    volatile_read<uint32_t>(SR);  // Reading SR clears PITS
  }

  inline uint32_t read_pimr() {
    return volatile_read<uint32_t>(PIMR);
  }

  // QEMU without icount runs extremely fast, so we aggregate many ticks into one.
  constexpr uint32_t HOST_TICK_DIVIDER = 200000;  // Large value keeps ticks sparse
  inline bool suppress_timer_output = false;
  inline void set_suppressed(bool value) { suppress_timer_output = value; }
  inline bool is_suppressed() {
    return suppress_timer_output;
  }
  inline volatile bool timer_tick_pending = false;
  inline void notify_tick() { timer_tick_pending = true; }
  inline bool consume_tick() {
    if (!timer_tick_pending) {
      return false;
    }
    timer_tick_pending = false;
    return true;
  }
  inline bool should_emit_tick() {
    static uint32_t tick_counter = 0;
    if (++tick_counter < HOST_TICK_DIVIDER) {
      return false;
    }
    tick_counter = 0;
    return true;
  }
} // namespace system_timer
