#include "./dbgu.hpp"
#include "util.hpp"
#include "interrupt.hpp"
#include "system_timer.hpp"

extern "C" __attribute__((section(".init"), noinline)) void main() {
  // Disable IRQs before touching peripherals
  __asm__ volatile(
    "mrs r0, cpsr\n"
    "orr r0, r0, #0x80\n"  // Set bit 7 to disable IRQ
    "msr cpsr_c, r0\n"
  );
  
  dbgu::init();
  
  // Allow UART to settle
  for (int i = 0; i < 1000000; i++) {
    __asm__ volatile("nop");
  }
  
  // Initialize interrupt hardware but keep timer disabled for now
  interrupt::init_hardware_interrupts();
  
  // Print banner before enabling IRQs
  dbgu::printf("Hello World\r\n");
  dbgu::flush();
  dbgu::printf("Task 3-2: Interrupt-driven system\r\n");
  dbgu::flush();
  dbgu::printf("System Timer: 100ms interval\r\n");
  dbgu::flush();
  dbgu::printf("Press keys to see calculation output\r\n");
  dbgu::flush();
  dbgu::printf("Timer interrupts will show '!'\r\n\r\n");
  dbgu::flush();
  
  // Long wait so every byte leaves the FIFO
  // 115200 baud = 11520 bytes/s, ~1152 chars/s
  // 200 chars ≈ 0.17 s, so wait ~1 s to be safe
  for (int i = 0; i < 100000000; i++) {
    __asm__ volatile("nop");
  }
  
  // Print initial prompt before starting timer
  dbgu::write_string("Waiting for key...\r\n");
  dbgu::flush();
  
  interrupt::start_system_timer();
  __asm__ volatile(
    "mrs r0, cpsr\n"
    "bic r0, r0, #0x80\n"
    "msr cpsr_c, r0\n"
  );

  for (;;) {
    dbgu::write_string("[MAIN LOOP]\r\n");
    dbgu::flush();
    dbgu::write_string("Waiting for key...\r\n");
    dbgu::flush();

    char c;
    while (!dbgu::pop_char(c)) {
      __asm__ volatile("nop");
    }

    dbgu::write_string("[MAIN KEY=0x");
    dbgu::write_hex(static_cast<unsigned int>(
        static_cast<unsigned char>(c)));
    dbgu::write_string("]\r\n");
    dbgu::flush();

    constexpr int REPEAT_COUNT = 25;
    constexpr int DELAY_LOOPS = 100000;
    
    for (int i = 0; i < REPEAT_COUNT; i++) {
      while (!(volatile_read<uint32_t>(dbgu::SR) & dbgu::TXRDY)) {
        __asm__ volatile("nop");
      }
      dbgu::write(c);
      
      for (int j = 0; j < DELAY_LOOPS; j++) {
        __asm__ volatile("nop");
      }
    }
    
    while (!(volatile_read<uint32_t>(dbgu::SR) & dbgu::TXRDY)) {
      __asm__ volatile("nop");
    }
    dbgu::write('\r');
    while (!(volatile_read<uint32_t>(dbgu::SR) & dbgu::TXRDY)) {
      __asm__ volatile("nop");
    }
    dbgu::write('\n');
  }
}