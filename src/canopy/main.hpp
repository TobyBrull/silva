#pragma once

#include "env_context.hpp"
#include "expected.hpp"

#define SILVA_MAIN(silva_main_func_name)                               \
  int main(const int argc, char* argv[])                               \
  {                                                                    \
    silva::env_context_t env_context_environ;                          \
    silva::env_context_fill_environ(&env_context_environ);             \
    silva::env_context_t env_context_cmdline;                          \
    silva::env_context_fill_cmdline(&env_context_cmdline, argc, argv); \
    const silva::expected_t<void> result = silva_main_func_name();     \
    if (!result) {                                                     \
      const silva::error_t& error = result.error();                    \
      fmt::print(stderr, "ERROR:\n{}\n", error.to_string());           \
      return static_cast<int>(error.level);                            \
    }                                                                  \
    else {                                                             \
      return 0;                                                        \
    }                                                                  \
  }
