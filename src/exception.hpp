#pragma once

#include <stdint.h>

namespace exception {
using VoidFunction = auto (*)(void) -> void;
// ARM.pdf page 160
extern "C" void init_exceptions();
} // namespace exception
