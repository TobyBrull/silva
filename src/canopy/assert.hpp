#pragma once

#include <fmt/base.h>
#include <fmt/format.h>

namespace silva {

#define SILVA_ASSERT_FMT(condition, msg, ...)                                              \
  do {                                                                                     \
    if (!(condition)) {                                                                    \
      silva::detail::maybe_fmt_assert_handler(__FILE__,                                    \
                                              __LINE__,                                    \
                                              __func__,                                    \
                                              FMT_STRING(msg) __VA_OPT__(, ) __VA_ARGS__); \
    }                                                                                      \
  } while (false)
#define SILVA_ASSERT(condition)                                                                  \
  do {                                                                                           \
    if (!(condition)) {                                                                          \
      silva::detail::maybe_fmt_assert_handler(__FILE__, __LINE__, __func__, "assertion failed"); \
    }                                                                                            \
  } while (false)
#define SILVA_TRY_ASSERT(x)                              \
  ({                                                     \
    auto result = (x);                                   \
    static_assert(is_expected<decltype(result)>::value); \
    SILVA_ASSERT(result);                                \
    std::move(result).value();                           \
  })
}

// IMPLEMENTATION

namespace silva::detail {
  template<typename... FS, typename... Args>
  void fmt_handler(char const* file,
                   long const line,
                   char const* func,
                   fmt::format_string<FS...> format,
                   Args const&... args)
  {
    fmt::print(stderr, FMT_STRING("silva: internal error in {} at {}:{} : "), func, file, line);
    fmt::print(stderr, format, args...);
    fmt::print(stderr, "\n");
  }

  template<typename... FS, typename... Args>
  [[noreturn]] void maybe_fmt_assert_handler(char const* file,
                                             long const line,
                                             char const* func,
                                             fmt::format_string<FS...> format,
                                             Args const&... args)
  {
    fmt_handler(file, line, func, format, args...);
    std::abort();
  }
}
