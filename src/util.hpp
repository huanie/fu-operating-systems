#pragma once
#include <stdint.h>

// Portux920t (AT91RM9200) master clock ≈49.152 MHz for DBGU and timers
constexpr uint32_t MASTER_CLOCK_HZ = 49'152'000;
template <typename T>
inline constexpr void volatile_write(uintptr_t addr, T value) {
  *reinterpret_cast<volatile T *>(addr) = value;
}

inline void __attribute__((naked)) no_operation() { __asm__ volatile("nop"); }

// Do not let the compiler reorder code
inline void barrier() { __asm__("" ::: "memory"); }

template <typename T> inline constexpr auto volatile_read(uintptr_t addr) -> T {
  return *reinterpret_cast<volatile T *>(addr);
}

inline void busy_wait(uint32_t usec) {
  // 180 MHz CPU speed
  // One iteration takes 2 instructions that need 4 clock cycles
  auto loops = usec * (180 / 4);
  while (loops--) {
    barrier();
  }
}

template <typename T>
constexpr bool is_char_type = __is_same(T, char) || __is_same(T, signed char) ||
                              __is_same(T, unsigned char);

template <typename T>
constexpr bool is_string_type =
    __is_same(T, const char *) || __is_same(T, char *);

template <typename T>
constexpr bool is_unsigned_int_type =
    __is_same(T, unsigned int) || __is_same(T, uint32_t) ||
    __is_same(T, uint8_t) || __is_same(T, uint64_t);

template <typename T>
constexpr bool is_void_ptr_type =
    __is_same(T, void *) || __is_same(T, const void *);
