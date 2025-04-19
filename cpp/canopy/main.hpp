#pragma once

#include "expected.hpp"

namespace silva {
  int main(const int argc, char* argv[], expected_t<void> (*)(span_t<string_view_t>));
}

#define SILVA_MAIN(silva_main_func_name)                   \
  int main(const int argc, char* argv[])                   \
  {                                                        \
    return silva::main(argc, argv, &silva_main_func_name); \
  }
