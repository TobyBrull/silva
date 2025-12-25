#pragma once

#include "string.hpp"

#include <fmt/format.h>

namespace silva {

  // ASSERTs maybe be disabled (e.g., in optimized builds). Therefore, they can only express
  // assumptions that will otherwise cause undefined behaviour. Prefer the use of
  // "SILVA_EXPECT(..., ASSERT)" if both of the following are true:
  //    * The containing function has an "expected_t" return-value anyway, or making the
  //      return-value "expected_t" would not be too awkward.
  //    * The runtime overhead incurred by the check and/or "expected_t" return-value is not a
  //      concern.

#define SILVA_ASSERT(condition, ...)                                                        \
  do {                                                                                      \
    if (!(condition)) {                                                                     \
      silva::impl::assert_handler(__FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__); \
    }                                                                                       \
  } while (false)
}

namespace silva::impl {
  [[noreturn]] void
  assert_handler_core(char const* file, const long line, char const* func, string_view_t);

  [[noreturn]] inline void assert_handler(char const* file, const long line, char const* func)
  {
    assert_handler_core(file, line, func, "assertion failed");
  }

  template<typename... Args>
  [[noreturn]] void assert_handler(char const* file,
                                   const long line,
                                   char const* func,
                                   fmt::format_string<Args...> fmt_str,
                                   Args&&... args)
  {
    assert_handler_core(file, line, func, fmt::format(fmt_str, std::forward<Args>(args)...));
  }
}
