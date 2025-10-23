#include "./dbgu.hpp"
extern "C" __attribute__((naked, section(".init"))) void _start() {
  dbgu::init();
  dbgu::write('h');
  dbgu::write('i');

  for (;;)
    ;
}
