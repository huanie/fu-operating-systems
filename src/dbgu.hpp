#pragma once
#include "constants.hpp"
#include "pio.hpp"
#include <stdint.h>

// AT91RM9200
// 26.5 Debug Unit User Interface
namespace dbgu {
uint32_t constexpr BAUDRATE = MASTER_CLOCK / (16 * ::BAUDRATE);
uint32_t constexpr BASE = 0xfffff200;
uint32_t constexpr CR = BASE + 0x0;
uint32_t constexpr MR = BASE + 0x4;
uint32_t constexpr IER = BASE + 0x8;
uint32_t constexpr IDR = BASE + 0xc;
uint32_t constexpr IMR = BASE + 0x10;
uint32_t constexpr SR = BASE + 0x14;
uint32_t constexpr RHR = BASE + 0x18;
uint32_t constexpr THR = BASE + 0x1c;
uint32_t constexpr BRGR = BASE + 0x20;
uint32_t constexpr CIDR = BASE + 0x40;
uint32_t constexpr EXID = BASE + 0x44;
// 26.5.1 Debug Unit Control Register
uint32_t constexpr RXEN = 1 << 4;
uint32_t constexpr RXDIS = 1 << 5;
uint32_t constexpr RSTRX = 1 << 2;
uint32_t constexpr TXEN = 1 << 6;
uint32_t constexpr TXDIS = 1 << 7;
uint32_t constexpr RSTTX = 1 << 3;
uint32_t constexpr CHMOD = 0;     // normal mode
uint32_t constexpr PAR = 1 << 11; // no parity
uint32_t constexpr TXRDY = 1 < 1;
inline void init() {
  // multiplexing: select peripheral, don't use the pin as GPIO
  volatile_write(PIOA + PIO_PDR, DBGU_PINS);
  // enable peripheral A
  volatile_write(PIOA + PIO_ASR, DBGU_PINS);

  // set baudrate
  volatile_write(BRGR, BAUDRATE);

  // set the mode
  volatile_write(MR, CHMOD | PAR);

  // reset transmitter and receiver, and enable
  volatile_write(CR, RSTTX | RSTRX | RXEN | TXEN);
}
inline void write(char character) {
  // wait until receiver is ready
  while (!(volatile_read<uint32_t>(SR) & TXRDY)) {
  }
  volatile_write<uint32_t>(THR, character);
}

} // namespace dbgu
