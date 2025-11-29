#pragma once
#include <stdint.h>

// Portux920t (AT91RM9200) master clock ≈49.152 MHz for DBGU and timers
constexpr uint32_t MASTER_CLOCK_HZ = 49'152'000;
template <typename T>
inline constexpr void volatile_write(uintptr_t addr, T value) {
  *reinterpret_cast<volatile T *>(addr) = value;
}

template <typename T> inline constexpr auto volatile_read(uintptr_t addr) -> T {
  return *reinterpret_cast<volatile T *>(addr);
}

template <typename T>
constexpr bool is_char_type = __is_same(T, char) || __is_same(T, signed char) ||
			      __is_same(T, unsigned char);

template <typename T>
constexpr bool is_string_type =
    __is_same(T, const char *) || __is_same(T, char *);

template <typename T>
constexpr bool is_unsigned_int_type =
    __is_same(T, unsigned int) || __is_same(T, uint32_t);

template <typename T>
constexpr bool is_void_ptr_type =
    __is_same(T, void *) || __is_same(T, const void *);
