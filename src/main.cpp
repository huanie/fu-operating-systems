#include "cpu.hpp"
#include "dbgu.hpp"
#include "stddef.h"
#include "system_timer.hpp"
#include "thread.hpp"

extern "C" __attribute__((section(".init"), noinline, noreturn)) void main() {
  thread::init();
  dbgu::init();
  system_timer::init();
  system_timer::set_interval(500);
  cpu::enable_irq();
  thread::idle_thread(0);
  // it should not go into this code
  while (true) {
  }
}
