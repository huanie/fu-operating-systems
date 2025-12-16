#include "system_timer.hpp"
#include "dbgu.hpp"
#include "schedule.hpp"

using namespace system_timer;
extern "C" [[gnu::used]] void system_timer::system_timer_interrupt() {
  if (volatile_read<uint32_t>(SR) & PITS) {
    dbgu::write('!');
    // the interrupt handler saves the context in the tcb (always)
    // end of the interrupt handler always restores context from tcb
    // here we switch the current thread, so restoring the context means that it
    // will switch to the thread
    schedule::select(schedule::next());
  }
}
