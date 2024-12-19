#pragma once

#include "error.hpp"

#include <expected>
#include <fmt/base.h>
#include <fmt/format.h>

namespace silva {
  template<typename T>
  using expected_t = std::expected<T, error_t>;

  template<typename T>
  struct is_expected_t : std::false_type {};

  template<typename T>
  struct is_expected_t<std::expected<T, error_t>> : std::true_type {};

#define SILVA_EXPECT(x, lvl, ...)                                                            \
  do {                                                                                       \
    using enum error_level_t;                                                                \
    static_assert(error_level_is_primary(lvl));                                              \
    if (!(x)) {                                                                              \
      return std::unexpected(silva::impl::make_parse_error(lvl __VA_OPT__(, ) __VA_ARGS__)); \
    }                                                                                        \
  } while (false)

#define SILVA_EXPECT_FWD(x)                                               \
  ({                                                                      \
    auto __silva_result = (x);                                            \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value); \
    if (!__silva_result) {                                                \
      return std::unexpected(std::move(__silva_result).error());          \
    }                                                                     \
    std::move(__silva_result).value();                                    \
  })

#define SILVA_EXPECT_FWD_WITH_AT_LEAST(x, new_lvl)                        \
  ({                                                                      \
    auto __silva_result = (x);                                            \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value); \
    if (!__silva_result) {                                                \
      using enum error_level_t;                                           \
      auto& __level = __silva_result.error().level;                       \
      __level       = std::max(new_lvl, __level);                         \
      return std::unexpected(std::move(__silva_result).error());          \
    }                                                                     \
    std::move(__silva_result).value();                                    \
  })

#define SILVA_EXPECT_FWD_IF(x, lvl)                                       \
  ({                                                                      \
    auto __silva_result = (x);                                            \
    using enum error_level_t;                                             \
    static_assert(error_level_is_primary(lvl));                           \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value); \
    if (!__silva_result && (__silva_result.error().level >= lvl)) {       \
      return std::unexpected(std::move(__silva_result).error());          \
    }                                                                     \
    std::move(__silva_result);                                            \
  })

// For Catch2
#define SILVA_EXPECT_REQUIRE(x)                                               \
  ({                                                                          \
    auto __silva_result = (x);                                                \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value);     \
    INFO((!__silva_result ? __silva_result.error().message.get_view() : "")); \
    REQUIRE(__silva_result);                                                  \
    std::move(__silva_result).value();                                        \
  })
}

// IMPLEMENTATION

namespace silva::impl {
  inline error_t make_parse_error(const error_level_t error_level)
  {
    return error_t{
        .level   = error_level,
        .message = string_or_view_t{string_view_t{"unexpected condition"}},
    };
  }

  inline error_t make_parse_error(const error_level_t error_level, string_view_t string_view)
  {
    return error_t{
        .level   = error_level,
        .message = string_or_view_t{string_view},
    };
  }

  template<typename... Args>
    requires(sizeof...(Args) > 0)
  error_t make_parse_error(const error_level_t error_level,
                           fmt::format_string<Args...> fmt_str,
                           Args&&... args)
  {
    return error_t{
        .level   = error_level,
        .message = string_or_view_t{fmt::format(fmt_str, std::forward<Args>(args)...)},
    };
  }
}
