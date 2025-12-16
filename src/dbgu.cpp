#include "dbgu.hpp"
#include "thread.hpp"
#include "util.hpp"
#include <stdint.h>

#define RX_BUFFSHIFT 5
#define RX_BUFFSIZE (1 << RX_BUFFSHIFT)
#define RX_BUFFMASK (RX_BUFFSIZE - 1)

static constinit char rx_buff[RX_BUFFSIZE];

static constinit volatile unsigned int rx_head = 0xfffffff8;
static constinit volatile unsigned int rx_tail = 0xfffffff8;
inline void rx_buff_putc(char c) {
  // ignore when buffer is full
  if (rx_head - rx_tail >= RX_BUFFSIZE) {
    return;
  }

  rx_buff[rx_head & RX_BUFFMASK] = c;
  rx_head += 1;
}

inline char rx_buff_getc(void) {
  // need barrier so that the compiler does not optimize this away
  while (rx_head - rx_tail == 0) {
    barrier();
  }
  auto c = rx_buff[rx_tail & RX_BUFFMASK];
  barrier();
  rx_tail += 1;

  return c;
}

[[gnu::noinline]] void dummy(uint8_t arg) {
  for (auto i = 0; i < 10; ++i) {
    dbgu::printf("%c", arg);
    busy_wait(500);
  }
}
#include "thread.hpp"
extern "C" [[gnu::used]] void dbgu::dbgu_interrupt() {
  // did we get a character?
  if (volatile_read<uint32_t>(dbgu::SR) & dbgu::RXRDY) {
    thread::create(dummy, volatile_read<char>(dbgu::RHR));
  }
}

char dbgu::read() { return rx_buff_getc(); }
