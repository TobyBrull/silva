#pragma once

#include "error.hpp"

#include <expected>
#include <format>

namespace silva {
  template<typename T>
  using expected_t = std::expected<T, error_t>;

  template<typename T>
  struct is_expected_t : std::false_type {};

  template<typename T>
  struct is_expected_t<std::expected<T, error_t>> : std::true_type {};

#define SILVA_EXPECT(x, ...)                                              \
  do {                                                                    \
    if (!(x)) {                                                           \
      return std::unexpected(silva::impl::make_parse_error(__VA_ARGS__)); \
    }                                                                     \
  } while (false)

#define SILVA_EXPECT_TRY(x)                                       \
  ({                                                              \
    auto result = (x);                                            \
    static_assert(silva::is_expected_t<decltype(result)>::value); \
    if (!result) {                                                \
      return std::unexpected(std::move(result).error());          \
    }                                                             \
    std::move(result).value();                                    \
  })

// For Catch2
#define SILVA_EXPECT_REQUIRE(x)                                   \
  ({                                                              \
    auto result = (x);                                            \
    static_assert(silva::is_expected_t<decltype(result)>::value); \
    INFO((!result ? result.error().message.get_view() : ""));     \
    REQUIRE(result);                                              \
    std::move(result).value();                                    \
  })
}

// IMPLEMENTATION

namespace silva::impl {
  inline error_t make_parse_error()
  {
    return error_t{string_or_view_t{string_view_t{"unexpected condition"}}};
  }

  inline error_t make_parse_error(string_view_t string_view)
  {
    return error_t{string_or_view_t{string_view}};
  }

  template<typename... Args>
    requires(sizeof...(Args) > 0)
  error_t make_parse_error(std::format_string<Args...> fmt_str, Args&&... args)
  {
    return error_t{string_or_view_t{std::format(fmt_str, std::forward<Args>(args)...)}};
  }
}
