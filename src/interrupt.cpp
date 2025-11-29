#include "interrupt.hpp"
#include "dbgu.hpp"
#include "util.hpp"
#include "aic.hpp"
#include "system_timer.hpp"
#include <stdint.h>

inline uint32_t get_spsr() {
  uint32_t spsr;
  __asm__ volatile("mrs %0, spsr" : "=r"(spsr));  // Move from System Register
  return spsr;
}
//get cpsr

inline uint32_t get_lr() {
  uint32_t lr;
  __asm__ volatile("mov %0, lr" : "=r"(lr));  // Copy LR into a C variable
  return lr;
}
//get lr

inline uint32_t get_far() {
  uint32_t far;
  // MRC: Move from Coprocessor Register
  __asm__ volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(far));
  return far;
}
//get far

inline uint32_t get_fsr() {
  uint32_t fsr;
  // c5, c0, 0 selects the FSR register
  __asm__ volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(fsr));
  return fsr;
}
//get fsr

void __attribute__((interrupt("ABORT"))) interrupt::data_abort() {
  dbgu::printf("abort!\n");
  uint32_t lr = get_lr();          
  uint32_t spsr = get_spsr();      
  uint32_t far = get_far();
  uint32_t fsr = get_fsr();
  uint32_t fault_pc = lr - 8;

  dbgu::printf("Exception Type: Data Abort\r\n");
  dbgu::printf("Fault Address (FAR): %p\r\n", (void*)far);  // Failing address
  dbgu::printf("Fault Status Register (FSR): %x\r\n", fsr);  // Fault status
  dbgu::printf("Fault PC (approximate): %p\r\n", (void*)fault_pc);  // Approx PC
  dbgu::printf("Link Register (LR): %p\r\n", (void*)lr);
  dbgu::printf("Saved CPSR (SPSR): %x\r\n", spsr);
  dbgu::printf("System halted.\r\n");

  for (;;) {
    __asm__ volatile("nop");  // Idle forever
  }
}

void __attribute__((interrupt("SWI"))) interrupt::software() {
  dbgu::printf("software!\n");
  uint32_t lr = get_lr();
  uint32_t spsr = get_spsr();

  uint32_t fault_pc = lr - 4;  // SWI: LR - 4
  
  dbgu::printf("Exception Type: Software Interrupt (SWI)\r\n");
  dbgu::printf("Fault PC (approximate): %p\r\n", (void*)fault_pc);
  dbgu::printf("Link Register (LR): %p\r\n", (void*)lr);
  dbgu::printf("Saved CPSR: %x\r\n", spsr);
  dbgu::printf("System halted.\r\n");
  
  for (;;) {
    __asm__ volatile("nop");
  }
}

void __attribute__((interrupt("UNDEF"))) interrupt::undefined_instruction() {
  dbgu::printf("Undefined instruction!\n");
  uint32_t lr = get_lr();
  uint32_t spsr = get_spsr();

  uint32_t fault_pc = lr - 4;  // Undefined Instruction: LR - 4
  
  dbgu::printf("Exception Type: Undefined Instruction\r\n");
  dbgu::printf("Fault PC (approximate): %p\r\n", (void*)fault_pc);
  dbgu::printf("Link Register (LR): %p\r\n", (void*)lr);
  dbgu::printf("Saved CPSR: %x\r\n", spsr);
  dbgu::printf("System halted.\r\n");
  
  for (;;) {
    __asm__ volatile("nop");
  }
}

template <uint32_t target, uint32_t destination>
constexpr inline uint32_t encode_load() {
  // +8 because that's the actual current instruction of the PC
  // we want to load the contents in destination address into PC
  auto constexpr pc = target + 8;
  auto constexpr offset = destination - pc;
  static_assert(offset < 0xFFF, "The destination is out of bounds");
  // the encoded load instruction
  constexpr uint32_t LOAD_INSTRUCTION = 0xE59FF000;
  return 0xE59FF000 | offset;
}

using handler_function = void (*)(void);
template <uint32_t target, uint32_t destination>
inline void install_interrupt_handler(handler_function handler) {
  volatile_write(target, encode_load<target, destination>());
  volatile_write(destination, reinterpret_cast<uint32_t>(handler));
}

constexpr uint32_t DATA_ABORT = 0x10;
constexpr uint32_t SOFTWARE = 0x8;
constexpr uint32_t UNDEFINED_INSTRUCTION = 0x4;
constexpr uint32_t IRQ = 0x18;

consteval uint32_t vector(int index) {
  // That's where the addresses of the interrupt handlers are stored
  constexpr uint32_t VECTOR = 0x30;
  return VECTOR + index * sizeof(uint32_t);
}
extern "C" void interrupt::init_interrupts() {
  // load instructions to load the handlers into PC
  install_interrupt_handler<DATA_ABORT, vector(0)>(&data_abort);
  install_interrupt_handler<SOFTWARE, vector(1)>(&software);
  install_interrupt_handler<UNDEFINED_INSTRUCTION, vector(2)>(
      &undefined_instruction);
}

void __attribute__((interrupt("IRQ"))) interrupt::irq_handler() {
  // AIC requires reading IVR before writing EOICR; get_current_interrupt does so
  uint32_t source_id = aic::get_current_interrupt();
  
  // Always check DBGU status in case multiple sources share the line
  uint32_t dbgu_status = volatile_read<uint32_t>(dbgu::SR);
  if (dbgu_status & dbgu::RXRDY) {
    char c = (char)volatile_read<uint32_t>(dbgu::RHR);
    dbgu::handle_rx_interrupt(c);
    // dbgu::write_string("[IRQ KEY=0x");
    // dbgu::write_hex(static_cast<unsigned int>(
    //     static_cast<unsigned char>(c)));
    // dbgu::write_string("]\r\n");
    dbgu::output_repeated_char(c);
  }
  
  // Handle system timer interrupt
  if (source_id == aic::IRQ_SOURCE_ST || system_timer::is_interrupt_pending()) {
    system_timer::clear_interrupt();
    if (system_timer::is_suppressed()) {
      aic::end_of_interrupt();
      return;
    }
    if (system_timer::should_emit_tick()) {
      dbgu::write('!');
      dbgu::write('\r');
      dbgu::write('\n');
      system_timer::notify_tick();
    }
  }
  
  // EOICR write must happen after IVR read
  aic::end_of_interrupt();
}

extern "C" void interrupt::init_hardware_interrupts() {
  install_interrupt_handler<IRQ, vector(3)>(&irq_handler);
  
  // Configure DBGU source first to receive keys
  aic::configure_source(aic::IRQ_SOURCE_DBGU, 
                       reinterpret_cast<uint32_t>(&irq_handler), 
                       6);
  aic::enable_source(aic::IRQ_SOURCE_DBGU);
  dbgu::enable_rx_interrupt();
  
  // Configure system timer source but leave it disabled for now
  aic::configure_source(aic::IRQ_SOURCE_ST, 
                       reinterpret_cast<uint32_t>(&irq_handler), 
                       5);
  // Timer source is enabled later inside start_system_timer()
  // aic::enable_source(aic::IRQ_SOURCE_ST);  // moved into start_system_timer()
}

extern "C" void interrupt::start_system_timer() {
  // Enable the AIC source first
  aic::enable_source(aic::IRQ_SOURCE_ST);
  // Then start the timer
  system_timer::init();
}