#include "dbgu.hpp"
#include "pio.hpp"
namespace dbgu {
void init() {
  // multiplexing: select peripheral, don't use the pin as GPIO
  volatile_write(PIOA + PIO_PDR, DBGU_PINS);
  // enable peripheral A
  volatile_write(PIOA + PIO_ASR, DBGU_PINS);

  // set baudrate
  volatile_write(BRGR, BAUDRATE);

  // set the mode
  volatile_write(MR, CHMOD | PAR);

  // reset transmitter and receiver, and enable
  volatile_write(CR, RSTTX | RSTRX | RXEN | TXEN);
}
void write(char character) {
  // wait until receiver is ready
  while (!(volatile_read(SR) & TXRDY)) {
  }
  volatile_write<uin32_t>(THR, character);
}
} // namespace dbgu
