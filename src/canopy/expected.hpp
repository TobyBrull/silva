#pragma once

#include "types.hpp"

#include <expected>
#include <format>

namespace silva {
  struct parse_error_t {
    string_t message;

    parse_error_t(std::string message) : message(std::move(message)) {}
  };

  template<typename T>
  using expected_t = std::expected<T, parse_error_t>;

  template<typename T>
  struct is_expected : std::false_type {};

  template<typename T>
  struct is_expected<std::expected<T, parse_error_t>> : std::true_type {};

#define SILVA_EXPECT(x, ...)                                              \
  do {                                                                    \
    if (!(x)) {                                                           \
      return std::unexpected(silva::impl::make_parse_error(__VA_ARGS__)); \
    }                                                                     \
  } while (false)

#define SILVA_TRY(x)                                            \
  ({                                                            \
    auto result = (x);                                          \
    static_assert(silva::is_expected<decltype(result)>::value); \
    if (!result) {                                              \
      return std::unexpected(std::move(result).error());        \
    }                                                           \
    std::move(result).value();                                  \
  })

// For Catch2
#define SILVA_TRY_REQUIRE(x)                                    \
  ({                                                            \
    auto result = (x);                                          \
    static_assert(silva::is_expected<decltype(result)>::value); \
    INFO((!result ? result.error().message : ""));              \
    REQUIRE(result);                                            \
    std::move(result).value();                                  \
  })
}

// IMPLEMENTATION

namespace silva::impl {
  inline parse_error_t make_parse_error()
  {
    return parse_error_t{"unexpected condition"};
  }

  template<typename... Args>
  parse_error_t make_parse_error(std::format_string<Args...> fmt_str, Args&&... args)
  {
    return parse_error_t{std::format(fmt_str, std::forward<Args>(args)...)};
  }
}
