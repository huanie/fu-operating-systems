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
@ save on the exception stack at first
push {r0-r12}
@ save sp and lr (user)
stmfd sp!, {r13, r14}^

@ get the current thread
ldr r0, =thread_current
ldr r0, [r0]

@ store cpsr
mrs r1, spsr
str r1, [r0, %[cpsr_offset]]

@ r1 has sp, r2 has lr (user)
ldmfd sp!, {r1, r2}
str r1, [r0, %[sp_offset]]
str r2, [r0, %[lr_offset]]

@ save r0-r11
ldmfd sp!, {r1-r12} @ pop value r0 into r1, r1 into r2, ...
stmia r0, {r1-r12}

@ save pc (lr irq, adjusted earlier)
str lr, [r0, %[pc_offset]]
@================== Handle interrupt ===============
bl system_timer_interrupt
bl dbgu_interrupt
@================== Restore context ================

@ load the new context
ldr r0, =thread_current
ldr r0, [r0]

@ Restore cpsr to spsr (will be restored to cpsr on return)
ldr r1, [r0, %[cpsr_offset]]
msr spsr_cxsf, r1

@ restore r0-r12 (r0 will be overwritten, so save TCB pointer in r1 temporarily)
mov r1, r0
@ load user mode sp and lr into IRQ mode registers before restoring r0-r12
ldr r2, [r1, %[sp_offset]]  @ load sp from TCB into r2 (IRQ mode)
ldr r3, [r1, %[lr_offset]]  @ load lr from TCB into r3 (IRQ mode)
@ push to IRQ stack so we can restore to user mode
stmfd sp!, {r2, r3}
@ now restore r0-r12 (this will overwrite r2 and r3, but we've already saved sp/lr)
ldmia r1!, {r0-r12}
@ restore user mode sp and lr from IRQ stack
ldmfd sp!, {r13, r14}^  @ restore to user mode sp and lr

@ TODO cannot just do that

@ restore pc and return from interrupt (restores cpsr from spsr)
ldr pc, [r1, #8]   @ pc is at offset 8 from r1 (r1 was incremented by ldmia)
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
