#include "./dbgu.hpp"

extern "C" __attribute__((naked, section(".init"))) void _start() {
  dbgu::init();
  //  dbgu::printf<"hello">();
  dbgu::printf("hello %x", 12u);
  for (;;)
    ;
}
