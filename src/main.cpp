#include "cpu.hpp"
#include "dbgu.hpp"
#include "system_timer.hpp"
#include "util.hpp"

extern "C" __attribute__((section(".init"), noinline)) void main() {
  dbgu::init();
  dbgu::printf("Hello World\n");
  system_timer::init();
  system_timer::set_interval(200);
  cpu::enable_irq();
  while (true) {
    auto c = dbgu::read();
    for (auto i = 0; i < 10; ++i) {
      dbgu::printf("%c", c);
      busy_wait(500);
    }
  }
}
