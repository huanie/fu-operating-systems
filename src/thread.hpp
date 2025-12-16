#pragma once

#include "circular_list.hpp"
#include <stddef.h>
#include <stdint.h>

namespace thread {
enum class State : uint8_t { done, running, waiting, ready };
using Handler = auto (*)(uint8_t) -> void;
struct ThreadControlBlock {
  // DO NOT REORDER
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r4;
  uint32_t r5;
  uint32_t r6;
  uint32_t r7;
  uint32_t r8;
  uint32_t r9;
  uint32_t r10;
  uint32_t r11;
  uint32_t r12;
  uint32_t sp;   // User mode SP (r13)
  uint32_t lr;   // User mode LR (r14)
  uint32_t pc;   // Return address (adjusted IRQ LR)
  uint32_t cpsr; // Saved CPSR (SPSR in IRQ mode)

  Handler handler;
  State state = State::done;
  uint8_t id;
  uint8_t arg;
};

constexpr auto NUMBER_OF_THREADS = 16;
constexpr auto STACK_SIZE = 512;

constexpr auto IDLE_THREAD_ID = 0;
constexpr auto IDLE_THREAD_CPSR = 0x1F; // system mode and irq enabled
constexpr auto NEW_THREAD_CPSR = 0x10;  // user mode and irq enabled

void idle_thread(uint8_t);

// start function for all threads
void start();

extern "C" uint32_t __stack_top_user;

/** Create a thread
 * @return -1 if unsuccessful (already NUMBER\_OF\_THREADS threads) otherwise
 * return the thread id
 */
int32_t create(Handler executor, uint8_t arg);
/**
 * Thread exit
 *
 * Sets the thread to done
 */
void exit();

// initialize the threads and create the idle thread
void init();
} // namespace thread

extern "C" CircularList<thread::ThreadControlBlock, thread::NUMBER_OF_THREADS>
    threads;
extern "C" decltype(&threads[0]) thread_current;
