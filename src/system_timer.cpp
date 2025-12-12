#include "system_timer.hpp"
#include "dbgu.hpp"

using namespace system_timer;
void system_timer::interrupt() {
  if (volatile_read<uint32_t>(SR) & PITS) {
    dbgu::printf("!\n");
  }
}
