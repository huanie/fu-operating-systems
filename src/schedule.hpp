#pragma once

#include "dbgu.hpp"
#include "thread.hpp"
#include <stdint.h>

namespace schedule {
// select the next thread id
inline auto next() -> uint8_t {
  auto current_id = thread_current->item.id;
  auto *iter = thread_current->next;

  while (iter->item.id != current_id) {
    if (iter->item.state == thread::State::ready &&
        // idle thread has lowest priority
        iter->item.id != thread::IDLE_THREAD_ID) {
      return iter->item.id;
    } else {
      iter = iter->next;
    }
  }
  return thread::IDLE_THREAD_ID;
}
inline void select(uint8_t id) {
  // switching to the same thread means nothing
  if (thread_current->item.id == id) {
    return;
  }
  dbgu::write('\n');
  thread_current = &threads[id];
}
} // namespace schedule
