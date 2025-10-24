#pragma once
#include <stdint.h>

template <typename T> inline void volatile_write(uintptr_t addr, T value) {
  *reinterpret_cast<volatile T *>(addr) = value;
}

template <typename T> inline auto volatile_read(uintptr_t addr) -> T {
  return *reinterpret_cast<volatile T *>(addr);
}

uint32_t constexpr MASTER_CLOCK = 49;
uint32_t constexpr BAUDRATE = 115200;
