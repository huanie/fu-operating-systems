#include "exception.hpp"
#include "aic.hpp"
#include "dbgu.hpp"
#include "thread.hpp"
#include "util.hpp"
#include <stddef.h>
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

using namespace thread;

[[gnu::naked]] void irq() {
  __asm__ volatile(R"(
@====================== Save context ====================

@ adjust the pc, the interrupt will set lr as the previous pc
sub lr, lr, #4
@ save on the exception stack at first because r12 is my tcb pointer
push {r12}

@ get the current thread
ldr r12, =thread_current
ldr r12, [r12]

@ save r0-r11
stmia r12!, {r0-r11} @ r12 is at tcb.r12
pop {r0} @ previous.r12 is in r0
stmia r12!, {r0}

@ store usr sp and lr
stmia r12, {sp, lr}^
add r12, r12, #8 @ manual advance because writeback! is not allowed in ^

@ save pc (lr irq, adjusted earlier)
stmia r12!, {lr}

@ save cpsr
mrs r1, spsr
stmia r12, {r1}

@================== Handle interrupt ===============
bl system_timer_interrupt
bl dbgu_interrupt
@================== Restore context ================

@ load the new context
ldr r0, =thread_current
ldr r0, [r0]

@ set the cpsr
ldr r1, [r0, %[cpsr_offset]]
msr spsr, r1

ldmia r0, {r0-r12, r13, r14, r15}^

                    )"
                   :
                   : [cpsr_offset] "i"(offsetof(ThreadControlBlock, cpsr)),
                     [sp_offset] "i"(offsetof(ThreadControlBlock, sp)),
                     [lr_offset] "i"(offsetof(ThreadControlBlock, lr)),
                     [pc_offset] "i"(offsetof(ThreadControlBlock, pc))
                   : "memory");
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
  install_exception_handler<IRQ, exception_vector(2)>(&irq);
  aic::init();
}
