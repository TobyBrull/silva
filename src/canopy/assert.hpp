#pragma once

#include "types.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

namespace silva {

#define SILVA_ASSERT_FMT(condition, msg, ...)                                     \
  do {                                                                            \
    if (!(condition)) {                                                           \
      silva::detail::assert_handler(__FILE__,                                     \
                                    __LINE__,                                     \
                                    __func__,                                     \
                                    fmt::format(msg __VA_OPT__(, ) __VA_ARGS__)); \
    }                                                                             \
  } while (false)
#define SILVA_ASSERT(condition)                                                        \
  do {                                                                                 \
    if (!(condition)) {                                                                \
      silva::detail::assert_handler(__FILE__, __LINE__, __func__, "assertion failed"); \
    }                                                                                  \
  } while (false)
#define SILVA_TRY_ASSERT(x)                                             \
  ({                                                                    \
    auto result = (x);                                                  \
    static_assert(is_expected<decltype(result)>::value);                \
    SILVA_ASSERT_FMT(result, "Unexpected: {}", result.error().message); \
    std::move(result).value();                                          \
  })
}

namespace silva::detail {
  [[noreturn]] void assert_handler(char const* file,
                                   long const line,
                                   char const* func,
                                   const string_t& error_message);
}
