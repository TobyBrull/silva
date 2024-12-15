#pragma once

#include "expected.hpp"

#define SILVA_MAIN(silva_main_func_name)                                     \
  int main(const int argc, char* argv[])                                     \
  {                                                                          \
    const expected_t<void> result = silva_main_func_name(argc, argv);        \
    if (!result) {                                                           \
      fmt::print(stderr, "ERROR:\n{}\n", result.error().message.get_view()); \
      return 1;                                                              \
    }                                                                        \
    else {                                                                   \
      return 0;                                                              \
    }                                                                        \
  }
