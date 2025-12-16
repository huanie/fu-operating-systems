#include "thread.hpp"
#include "dbgu.hpp"
#include <stdint.h>
__attribute__((noinline, noreturn)) void thread::idle_thread(uint8_t) {
  while (1) {
    dbgu::write('t');
  }
}

[[gnu::used]] CircularList<thread::ThreadControlBlock,
                           thread::NUMBER_OF_THREADS> threads;
[[gnu::used]] decltype(&threads[0]) thread_current =
    &threads[thread::IDLE_THREAD_ID];

[[gnu::noreturn, gnu::noinline]] void thread::exit() {
  thread_current->item.state = thread::State::done;
  // too lazy, just wait for timer interrupt to change to a new thread
  while (true) {
  }
}

/// This should only be called on the start of the thread
/// pc will point to the function that the thread wants to run
/// after that pc will be the instruction pointer for context switching
[[gnu::noreturn, gnu::noinline]] void thread::start() {
  thread_current->item.handler(thread_current->item.arg);
  thread::exit();
}

int32_t thread::create(Handler executor, uint8_t arg) {
  auto base = reinterpret_cast<uintptr_t>(&__stack_top_user);
  for (auto i = 0; i < NUMBER_OF_THREADS; ++i) {
    if (threads[i].item.state == State::done) {
      // we use lr to jump back to the correct code
      // +4 because there is no difference in starting and continuing a thread
      // subs pc, lr, #4
      //
      threads[i].item.lr = reinterpret_cast<uint32_t>(start);
      threads[i].item.handler = executor;
      threads[i].item.pc = reinterpret_cast<uint32_t>(start);
      threads[i].item.state = State::ready;
      threads[i].item.arg = arg;
      threads[i].item.sp = base - i * STACK_SIZE;
      threads[i].item.cpsr = NEW_THREAD_CPSR;
      return i;
    }
  }
  return -1;
}

void thread::init() {
  auto base = reinterpret_cast<uintptr_t>(&__stack_top_user);
  for (auto i = 0; i < NUMBER_OF_THREADS; ++i) {
    threads[i].item.id = i;
    threads[i].item.sp = base - i * STACK_SIZE;
    threads[i].item.cpsr = NEW_THREAD_CPSR;
    threads[i].item.pc = reinterpret_cast<uint32_t>(start);
    threads[i].item.lr = reinterpret_cast<uint32_t>(start);
  }
  auto &idle = threads[IDLE_THREAD_ID].item;
  // we use lr to jump back to the correct code
  // +4 because there is no difference in starting and continuing a thread
  // subs pc, lr, #4
  idle.handler = idle_thread;
  idle.state = State::ready;
}
