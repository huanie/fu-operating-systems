#pragma once
#include "pio.hpp"
#include "util.hpp"
#include <stdint.h>
// AT91RM9200
// 26.5 Debug Unit User Interface
namespace dbgu {
constexpr uint32_t BAUD_RATE = 115200;
constexpr uint32_t BRGR_VALUE =
    (MASTER_CLOCK_HZ + (BAUD_RATE * 8)) / (BAUD_RATE * 16);  // rounded value
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
uint32_t constexpr TXRDY = 1 << 1;
uint32_t constexpr FRAME = 1 << 6;
uint32_t constexpr RSTSTA = 1 << 8;
uint32_t constexpr OVRE = 1 << 5;
uint32_t constexpr RXRDY = 1 << 0;
inline void init() {
  // multiplexing: select peripheral, don't use the pin as GPIO
  volatile_write(PIOA + PIO_PDR, DBGU_PINS);
  // enable peripheral A
  volatile_write(PIOA + PIO_ASR, DBGU_PINS);

  // set baudrate
  volatile_write(BRGR, BRGR_VALUE);

  // set the mode
  volatile_write(MR, CHMOD | PAR);

  // reset transmitter and receiver, and enable
  volatile_write(CR, RSTTX | RSTRX | RXEN | TXEN);
}
inline void write(char character) {
  // wait until transmitter is ready
  while (!(volatile_read<uint32_t>(SR) & TXRDY)) {
  }
  volatile_write<uint32_t>(THR, character);
}

// Wait until TX buffer is empty
inline void flush() {
  // Wait for TX shift register to drain
  // Ensures the last byte is transmitted
  while (!(volatile_read<uint32_t>(SR) & TXRDY)) {
  }
  // Small delay to guarantee completion
  for (int i = 0; i < 10000; i++) {
    __asm__ volatile("nop");
  }
}

// String version
inline void write_string(const char *str) {
  if (!str) {
    write_string("(null)");
    return;
  }
  while (*str) {
    write(*str++);
  }
}

// Hex version
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
// base case
inline void printf(const char *str) {
  while (*str) {
    write(*str);
    str++;
  }
}

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

namespace {
  constexpr uint8_t RX_BUFFER_SIZE = 64;
  inline volatile char g_rx_buffer[RX_BUFFER_SIZE];
  inline volatile uint8_t g_rx_head = 0;
  inline volatile uint8_t g_rx_tail = 0;
  inline volatile bool g_rx_overflow = false;
}

inline void enable_rx_interrupt() {
  volatile_write(IER, RXRDY);
}

inline void handle_rx_interrupt(char c) {
  uint8_t next = (g_rx_head + 1) % RX_BUFFER_SIZE;
  if (next == g_rx_tail) {
    g_rx_overflow = true;
    return;
  }
  g_rx_buffer[g_rx_head] = c;
  g_rx_head = next;
}

inline bool pop_char(char &c) {
  if (g_rx_head == g_rx_tail) {
    return false;
  }
  c = g_rx_buffer[g_rx_tail];
  g_rx_tail = (g_rx_tail + 1) % RX_BUFFER_SIZE;
  return true;
}

inline bool has_pending_char() {
  return g_rx_head != g_rx_tail;
}

inline void output_repeated_char(char c, int repeat = 25, int delay_loops = 50000) {
  for (int i = 0; i < repeat; i++) {
    write(c);
    for (int j = 0; j < delay_loops; j++) {
      __asm__ volatile("nop");
    }
  }
  write('\r');
  write('\n');
}

} // namespace dbgu