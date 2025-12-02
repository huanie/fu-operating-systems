#pragma once

#include <stdint.h>

namespace interrupt {
// ARM.pdf page 160
constexpr int32_t BRANCH_INSTRUCTION_OP_CODE = 0xEA000000;
extern "C" void init_interrupts();
void data_abort();
void software();
void undefined_instruction();
extern "C" void irq_handler();
extern "C" void init_hardware_interrupts();
extern "C" void start_system_timer();
} // namespace interrupt