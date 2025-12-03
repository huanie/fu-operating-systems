#pragma once
#include "pio.hpp"
#include "util.hpp"
#include <stdint.h>

// AT91RM9200
// 26.5 Debug Unit User Interface

// PIO (Parallel I/O) controller definitions
// Define necessary constants here since pio.hpp was removed
namespace pio {
  constexpr uint32_t PIOA = 0xfffff400;      // Base address of PIO controller A
  constexpr uint32_t PIO_PDR = 0x4;          // Peripheral Disable Register offset
  constexpr uint32_t PIO_ASR = 0x70;         // Peripheral Select Register A offset
  constexpr uint32_t DBGU_RX_PIN = 1 << 30;   // Receive pin (bit 30)
  constexpr uint32_t DBGU_TX_PIN = 1 << 31;   // Transmit pin (bit 31)
  constexpr uint32_t DBGU_PINS = DBGU_RX_PIN | DBGU_TX_PIN;  // Combined pins
}

class DBGU {
public:
  // Disable instantiation, copy and move
  // Hardware peripheral driver, use as utility class (static members and methods only)
  DBGU() = delete;                              // Disable default constructor
  DBGU(const DBGU&) = delete;                    // Disable copy constructor
  DBGU& operator=(const DBGU&) = delete;         // Disable copy assignment
  DBGU(DBGU&&) = delete;                         // Disable move constructor
  DBGU& operator=(DBGU&&) = delete;              // Disable move assignment

  // Constant definitions
  static constexpr uint32_t BAUD_RATE = 115200;
  static constexpr uint32_t BRGR_VALUE =
      (MASTER_CLOCK_HZ + (BAUD_RATE * 8)) / (BAUD_RATE * 16);  // rounded value
  
  // Register addresses
  static constexpr uint32_t BASE = 0xfffff200;
  static constexpr uint32_t CR = BASE + 0x0;
  static constexpr uint32_t MR = BASE + 0x4;
  static constexpr uint32_t IER = BASE + 0x8;
  static constexpr uint32_t IDR = BASE + 0xc;
  static constexpr uint32_t IMR = BASE + 0x10;
  static constexpr uint32_t SR = BASE + 0x14;
  static constexpr uint32_t RHR = BASE + 0x18;
  static constexpr uint32_t THR = BASE + 0x1c;
  static constexpr uint32_t BRGR = BASE + 0x20;
  static constexpr uint32_t CIDR = BASE + 0x40;
  static constexpr uint32_t EXID = BASE + 0x44;
  
  // Control bit definitions
  static constexpr uint32_t RXEN = 1 << 4;
  static constexpr uint32_t RXDIS = 1 << 5;
  static constexpr uint32_t RSTRX = 1 << 2;
  static constexpr uint32_t TXEN = 1 << 6;
  static constexpr uint32_t TXDIS = 1 << 7;
  static constexpr uint32_t RSTTX = 1 << 3;
  static constexpr uint32_t CHMOD = 0;     // normal mode
  static constexpr uint32_t PAR = 1 << 11; // no parity
  static constexpr uint32_t TXRDY = 1 << 1;
  static constexpr uint32_t FRAME = 1 << 6;
  static constexpr uint32_t RSTSTA = 1 << 8;
  static constexpr uint32_t OVRE = 1 << 5;
  static constexpr uint32_t RXRDY = 1 << 0;

  // Receive buffer constants
  static constexpr uint8_t RX_BUFFER_SIZE = 64;

  inline static volatile char rx_buffer[RX_BUFFER_SIZE] = {};
  inline static volatile uint8_t rx_head = 0;
  inline static volatile uint8_t rx_tail = 0;
  inline static volatile bool rx_overflow = false;

  // Initialization function
  static void init() {
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
  }

  // Write single character
  static void write(char character) {
    // wait until transmitter is ready
    while (!(volatile_read<uint32_t>(SR) & TXRDY)) {
    }
    volatile_write<uint32_t>(THR, character);
  }

  // Wait for transmit buffer to empty
  static void flush() {
    // Wait for TX shift register to drain
    // Ensures the last byte is transmitted
    while (!(volatile_read<uint32_t>(SR) & TXRDY)) {
    }
    // Small delay to guarantee completion
    for (int i = 0; i < 10000; i++) {
      __asm__ volatile("nop");
    }
  }

  // Write string
  static void write_string(const char *str) {
    if (!str) {
      write_string("(null)");
      return;
    }
    while (*str) {
      write(*str++);
    }
  }

  // Write hexadecimal number
  static void write_hex(unsigned int value) {
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
  static void printf(const char *str) {
    while (*str) {
      write(*str);
      str++;
    }
  }

  // printf template version (supports formatting)
  template <typename T, typename... Args>
  static void printf(const char *format, T value, Args... args) {
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

  // Enable receive interrupt
  static void enable_rx_interrupt() {
    volatile_write(IER, RXRDY);
  }

  // Handle receive interrupt
  static void handle_rx_interrupt(char c) {
    uint8_t next = (rx_head + 1) % RX_BUFFER_SIZE;
    if (next == rx_tail) {
      rx_overflow = true;
      return;
    }
    rx_buffer[rx_head] = c;
    rx_head = next;
  }

  // Pop character from buffer
  static bool pop_char(char &c) {
    if (rx_head == rx_tail) {
      return false;
    }
    c = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    return true;
  }

  // Check if there are pending characters
  static bool has_pending_char() {
    return rx_head != rx_tail;
  }

  // Output repeated character (for testing)
  static void output_repeated_char(char c, int repeat = 25, int delay_loops = 50000) {
    for (int i = 0; i < repeat; i++) {
      write(c);
      for (int j = 0; j < delay_loops; j++) {
        __asm__ volatile("nop");
      }
    }
    write('\r');
    write('\n');
  }
};

// Provide namespace wrapper for backward compatibility
// So existing dbgu:: calls still work
namespace dbgu {
  // Provide convenient aliases so code can continue using dbgu::init() etc
  inline void init() { DBGU::init(); }
  inline void write(char c) { DBGU::write(c); }
  inline void flush() { DBGU::flush(); }
  inline void write_string(const char *str) { DBGU::write_string(str); }
  inline void write_hex(unsigned int value) { DBGU::write_hex(value); }
  inline void printf(const char *str) { DBGU::printf(str); }
  template <typename T, typename... Args>
  inline void printf(const char *format, T value, Args... args) {
    DBGU::printf(format, value, args...);
  }
  inline void enable_rx_interrupt() { DBGU::enable_rx_interrupt(); }
  inline void handle_rx_interrupt(char c) { DBGU::handle_rx_interrupt(c); }
  inline bool pop_char(char &c) { return DBGU::pop_char(c); }
  inline bool has_pending_char() { return DBGU::has_pending_char(); }
  inline void output_repeated_char(char c, int repeat = 25, int delay_loops = 50000) {
    DBGU::output_repeated_char(c, repeat, delay_loops);
  }
  
  // Constant aliases
  constexpr uint32_t SR = DBGU::SR;
  constexpr uint32_t RHR = DBGU::RHR;
  constexpr uint32_t TXRDY = DBGU::TXRDY;
  constexpr uint32_t RXRDY = DBGU::RXRDY;
}