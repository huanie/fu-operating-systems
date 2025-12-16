#pragma once
#include <stdint.h>

template <typename T, auto Size> struct CircularList {
  struct Node {
    T item = {};
    Node *next;
  };
  Node list[Size];
  consteval CircularList() {
    list[Size - 1].next = &list[0];
    for (auto i = 0; i < Size - 1; ++i) {
      list[i].next = &list[i + 1];
    }
  }
  [[nodiscard]] inline constexpr uint32_t length() { return Size; }
  [[nodiscard]] auto inline constexpr head() -> Node & { return list[0]; }
  [[nodiscard]] auto inline constexpr head() const -> const Node & {
    return list[0];
  }
  [[nodiscard]] auto inline constexpr tail() -> Node & {
    return list[Size - 1];
  }
  [[nodiscard]] auto inline constexpr tail() const -> const Node & {
    return list[Size - 1];
  }
  auto inline constexpr operator[](uint32_t index) -> Node & {
    return list[index % Size];
  }
  auto inline constexpr operator[](uint32_t index) const -> const Node & {
    return list[index % Size];
  }
};
