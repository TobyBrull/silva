#pragma once

#include "types.hpp"

#include <format>

namespace silva {

  // ASSERTs maybe be disabled (e.g., in optimized builds). Therefore, they can only express
  // assumptions that will otherwise cause undefined behaviour. In functions that have an
  // "expected_t" return value, and where performance is never an issue, prefer to use
  // "SILVA_EXPECT(..., ASSERT)" to express the same thing.

#define SILVA_ASSERT(condition, ...)                                                        \
  do {                                                                                      \
    if (!(condition)) {                                                                     \
      silva::impl::assert_handler(__FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__); \
    }                                                                                       \
  } while (false)

#define SILVA_ASSERT_EXPECTED(x)                                               \
  ({                                                                           \
    auto result = (x);                                                         \
    static_assert(is_expected_t<decltype(result)>::value);                     \
    SILVA_ASSERT(result, "Unexpected: {}", result.error().message.get_view()); \
    std::move(result).value();                                                 \
  })
}

namespace silva::impl {
  [[noreturn]] void
  assert_handler_core(char const* file, long const line, char const* func, string_view_t);

  [[noreturn]] inline void assert_handler(char const* file, long const line, char const* func)
  {
    assert_handler_core(file, line, func, "assertion failed");
  }

  template<typename... Args>
  [[noreturn]] void assert_handler(char const* file,
                                   long const line,
                                   char const* func,
                                   std::format_string<Args...> fmt_str,
                                   Args&&... args)
  {
    assert_handler_core(file, line, func, std::format(fmt_str, std::forward<Args>(args)...));
  }
}
