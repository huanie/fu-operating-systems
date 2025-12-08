#pragma once
#include "aic.hpp"
#include "pio.hpp"
#include "util.hpp"
#include <stdint.h>

// AT91RM9200
// 26.5 Debug Unit User Interface
namespace dbgu {
constexpr uint32_t BAUD_RATE = 115200;
constexpr uint32_t BRGR_VALUE =
    (MASTER_CLOCK_HZ + (BAUD_RATE * 8)) / (BAUD_RATE * 16); // rounded value

// Register addresses
constexpr uint32_t BASE = 0xfffff200;
constexpr uint32_t CR = BASE + 0x0;
constexpr uint32_t MR = BASE + 0x4;
constexpr uint32_t IER = BASE + 0x8;
constexpr uint32_t IDR = BASE + 0xc;
constexpr uint32_t IMR = BASE + 0x10;
constexpr uint32_t SR = BASE + 0x14;
constexpr uint32_t RHR = BASE + 0x18;
constexpr uint32_t THR = BASE + 0x1c;
constexpr uint32_t BRGR = BASE + 0x20;
constexpr uint32_t CIDR = BASE + 0x40;
constexpr uint32_t EXID = BASE + 0x44;

// Control bit definitions
constexpr uint32_t RXEN = 1 << 4;
constexpr uint32_t RXDIS = 1 << 5;
constexpr uint32_t RSTRX = 1 << 2;
constexpr uint32_t TXEN = 1 << 6;
constexpr uint32_t TXDIS = 1 << 7;
constexpr uint32_t RSTTX = 1 << 3;
constexpr uint32_t CHMOD = 0;     // normal mode
constexpr uint32_t PAR = 1 << 11; // no parity
constexpr uint32_t TXRDY = 1 << 1;
constexpr uint32_t FRAME = 1 << 6;
constexpr uint32_t RSTSTA = 1 << 8;
constexpr uint32_t OVRE = 1 << 5;
constexpr uint32_t RXRDY = 1 << 0;

inline void init() {
  // multiplexing: select peripheral, don't use the pin as GPIO
  volatile_write(pio::PIOA + pio::PIO_PDR, pio::DBGU_PINS);
  // enable peripheral A
  volatile_write(pio::PIOA + pio::PIO_ASR, pio::DBGU_PINS);

  // set baudrate
  volatile_write(BRGR, BRGR_VALUE);

  // set the mode
  volatile_write(MR, CHMOD | PAR);

  // reset transmitter and receiver, and enable
  volatile_write(CR, RSTTX | RSTRX | RXEN | TXEN);

  // enable receive interrupt
  volatile_write(IER, RXRDY);

  // register the interrupt
  aic::enable_interrupt<aic::SYSIRQ>();
}

void interrupt();

char read();

// Write single character
inline void write(char character) {
  // wait until transmitter is ready
  while (!(volatile_read<uint32_t>(SR) & TXRDY)) {
  }
  volatile_write<uint32_t>(THR, character);
}

// Write string
inline void write_string(const char *str) {
  if (!str) {
    write_string("(null)");
    return;
  }
  while (*str) {
    write(*str++);
  }
}

// Write hexadecimal number
inline void write_hex(unsigned int value) {
  write('0');
  write('x');
  constexpr char hex_chars[] = "0123456789ABCDEF";
  bool started = false;
  // Print each nibble from highest to lowest
  for (int shift = (sizeof(unsigned int) * 8) - 4; shift >= 0; shift -= 4) {
    unsigned int nibble = (value >> shift) & 0xF;
    if (nibble != 0 || started || shift == 0) { // Avoid leading zeros
      char c = hex_chars[nibble];
      write(c);
      started = true;
    }
  }
}

// printf base version (no parameters)
inline void printf(const char *str) {
  while (*str) {
    write(*str);
    str++;
  }
}

// printf template version (supports formatting)
template <typename T, typename... Args>
inline void printf(const char *format, T value, Args... args) {
  static_assert(is_char_type<T> || is_string_type<T> || is_void_ptr_type<T> ||
                    is_unsigned_int_type<T>,
                "Formatter is not supported");
  while (*format) {
    if (*format == '%') {
      auto formatter = format + 1;
      format += 2; // skip the % and also skip the formatter
      if (!formatter) {
        return;
      }
      if (*formatter == 'c') {
        if constexpr (is_char_type<T>) {
          write(value);
        }
      } else if (*formatter == 's') {
        if constexpr (is_string_type<T>) {
          write_string(value);
        }
      } else if (*formatter == 'p') {
        if constexpr (is_void_ptr_type<T>) {
          write_hex(reinterpret_cast<unsigned int>(value));
        }
      } else if (*formatter == 'x') {
        if constexpr (is_unsigned_int_type<T>) {
          write_hex(value);
        }
      } else {
        // unknown formatter
        write('%');
      }
      printf(format, args...);
      return;
    } else {
      write(*format);
      ++format;
    }
  }
}
} // namespace dbgu
