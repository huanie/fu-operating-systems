#pragma once
#include "util.hpp"
#include <stdint.h>

namespace aic {
  constexpr uint32_t BASE = 0xFFFFF000;

  constexpr uint32_t SMR = BASE + 0x00;   // Source Mode Register (32 sources, 4 bytes)
  constexpr uint32_t SVR = BASE + 0x80;   // Source Vector Register (32 sources, 4 bytes)
  constexpr uint32_t IVR = BASE + 0x100;  // Interrupt Vector Register
  constexpr uint32_t FVR = BASE + 0x104;  // Fast Interrupt Vector Register
  constexpr uint32_t ISR = BASE + 0x108;  // Interrupt Status Register
  constexpr uint32_t IPR = BASE + 0x10C;  // Interrupt Pending Register
  constexpr uint32_t IMR = BASE + 0x110;  // Interrupt Mask Register
  constexpr uint32_t CISR = BASE + 0x114; // Core Interrupt Status Register
  constexpr uint32_t IECR = BASE + 0x120; // Interrupt Enable Command Register
  constexpr uint32_t IDCR = BASE + 0x124; // Interrupt Disable Command Register
  constexpr uint32_t ICCR = BASE + 0x128; // Interrupt Clear Command Register
  constexpr uint32_t ISCR = BASE + 0x12C; // Interrupt Set Command Register
  constexpr uint32_t EOICR = BASE + 0x130; // End of Interrupt Command Register
  
  constexpr uint32_t IRQ_SOURCE_ST = 1;      // System Timer
  constexpr uint32_t IRQ_SOURCE_DBGU = 2;    // DBGU
  
  constexpr uint32_t SMR_SRCTYPE = 1 << 5;   // 0=level-sensitive, 1=edge-triggered

  inline void configure_source(uint32_t source_id, uint32_t handler, uint32_t priority) {
    volatile_write(SMR + source_id * 4, priority);
    
    volatile_write(SVR + source_id * 4, handler);
  }
  
  inline void enable_source(uint32_t source_id) {
    volatile_write(IECR, 1 << source_id);
  }
  
  inline void disable_source(uint32_t source_id) {
    volatile_write(IDCR, 1 << source_id);
  }
  
  inline uint32_t get_current_interrupt() {
    uint32_t current_svr = volatile_read<uint32_t>(IVR);
    
    uint32_t st_svr = volatile_read<uint32_t>(SVR + IRQ_SOURCE_ST * 4);
    uint32_t dbgu_svr = volatile_read<uint32_t>(SVR + IRQ_SOURCE_DBGU * 4);
    
    if (current_svr == st_svr) {
      return IRQ_SOURCE_ST;
    } else if (current_svr == dbgu_svr) {
      return IRQ_SOURCE_DBGU;
    }
    
    return 0xFFFFFFFF;
  }
  
inline void end_of_interrupt() {
    volatile_write(EOICR, 0);
  }
} 