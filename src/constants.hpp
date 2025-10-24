#pragma once
#include <stdint.h>

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
    __is_same(T, unsigned int) || __is_same(T, const unsigned int);

template <typename T>
constexpr bool is_void_ptr_type =
    __is_same(T, void *) || __is_same(T, const void *);

uint32_t constexpr MASTER_CLOCK = 49;
uint32_t constexpr BAUDRATE = 115200;
