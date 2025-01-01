#pragma once

#include "env_context.hpp"
#include "expected.hpp"

#define SILVA_MAIN_IMPL_PRE                                \
  int main(const int argc, char* argv[])                   \
  {                                                        \
    silva::env_context_t env_context_environ;              \
    silva::env_context_fill_environ(&env_context_environ); \
    silva::env_context_t env_context_cmdline;              \
    silva::env_context_fill_cmdline(&env_context_cmdline, argc, argv);

#define SILVA_MAIN_IMPL_POST                                                            \
  if (!result) {                                                                        \
    const silva::error_t& error = result.error();                                       \
    fmt::print(stderr, "ERROR ({}):\n{}\n", to_string(error.level), error.to_string()); \
    return static_cast<int>(error.level);                                               \
  }                                                                                     \
  else {                                                                                \
    return 0;                                                                           \
  }                                                                                     \
  }

#define SILVA_MAIN(silva_main_func_name)                         \
  SILVA_MAIN_IMPL_PRE                                            \
  const silva::expected_t<void> result = silva_main_func_name(); \
  SILVA_MAIN_IMPL_POST

#define SILVA_MAIN_1(silva_main_func_name)                                   \
  SILVA_MAIN_IMPL_PRE                                                        \
  vector_t<string_view_t> cmdline_args;                                      \
  for (int i = 0; i < argc; ++i) {                                           \
    cmdline_args.push_back(string_view_t{argv[i]});                          \
  }                                                                          \
  const silva::expected_t<void> result = silva_main_func_name(cmdline_args); \
  SILVA_MAIN_IMPL_POST
