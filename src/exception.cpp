#include "exception.hpp"
#include "aic.hpp"
#include "dbgu.hpp"
#include "system_timer.hpp"
#include "util.hpp"
#include <stdint.h>

using namespace exception;

void __attribute__((interrupt("ABORT"))) data_abort() {
  dbgu::printf("abort\r\n");
}

void __attribute__((interrupt("SWI"))) software() {
  dbgu::printf("software!\n");
}

void __attribute__((interrupt("UNDEF"))) undefined_instruction() {
  dbgu::printf("Undefined instruction!\n");
}

void __attribute__((interrupt("IRQ"))) irq() {
  aic::start();
  system_timer::interrupt();
  dbgu::interrupt();
  aic::end();
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

template <uint32_t target, uint32_t destination>
inline void install_exception_handler(VoidFunction handler) {
  volatile_write(target, encode_load<target, destination>());
  volatile_write(destination, reinterpret_cast<uint32_t>(handler));
}

constexpr uint32_t DATA_ABORT = 0x10;
constexpr uint32_t SOFTWARE = 0x8;
constexpr uint32_t UNDEFINED_INSTRUCTION = 0x4;
constexpr uint32_t IRQ = 0x18;

consteval uint32_t exception_vector(int index) {
  // That's where the addresses of the exception handlers are stored
  constexpr uint32_t VECTOR = 0x30;
  return VECTOR + index * sizeof(uint32_t);
}

constexpr uint32_t INTERRUPT_VECTOR_TRAMPOLINE = 0xe51fff20;

extern "C" void exception::init_exceptions() {
  // load instructions to load the handlers into PC
  install_exception_handler<DATA_ABORT, exception_vector(0)>(&data_abort);
  install_exception_handler<SOFTWARE, exception_vector(1)>(&software);
  install_exception_handler<UNDEFINED_INSTRUCTION, exception_vector(2)>(
      &undefined_instruction);
  volatile_write(IRQ, INTERRUPT_VECTOR_TRAMPOLINE);
  aic::enable_interrupt<aic::SYSIRQ>(irq);
}
