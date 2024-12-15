#pragma once

#include "types.hpp"

#include <format>

namespace silva {

#define SILVA_ASSERT(condition, ...)                                                        \
  do {                                                                                      \
    if (!(condition)) {                                                                     \
      silva::impl::assert_handler(__FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__); \
    }                                                                                       \
  } while (false)

#define SILVA_ASSERT_EXPECTED(x)                                    \
  ({                                                                \
    auto result = (x);                                              \
    static_assert(is_expected_t<decltype(result)>::value);          \
    SILVA_ASSERT(result, "Unexpected: {}", result.error().message); \
    std::move(result).value();                                      \
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
