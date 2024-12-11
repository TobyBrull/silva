#pragma once

#include "types.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <expected>

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

#define SILVA_EXPECT(x)                                              \
  do {                                                               \
    if (!(x)) {                                                      \
      return std::unexpected(parse_error_t{"unexpected condition"}); \
    }                                                                \
  } while (false)

#define SILVA_EXPECT_FMT(x, msg, ...)                                                     \
  do {                                                                                    \
    if (!(x)) {                                                                           \
      return std::unexpected(parse_error_t{fmt::format(msg __VA_OPT__(, ) __VA_ARGS__)}); \
    }                                                                                     \
  } while (false)

#define SILVA_UNEXPECTED()        SILVA_EXPECT(false);
#define SILVA_UNEXPECTED_FMT(...) SILVA_EXPECT_FMT(false, __VA_ARGS__);

#define SILVA_TRY(x)                                     \
  ({                                                     \
    auto result = (x);                                   \
    static_assert(is_expected<decltype(result)>::value); \
    if (!result) {                                       \
      return std::unexpected(std::move(result).error()); \
    }                                                    \
    std::move(result).value();                           \
  })

#define SILVA_TRY_REQUIRE(x)                             \
  ({                                                     \
    auto result = (x);                                   \
    static_assert(is_expected<decltype(result)>::value); \
    INFO((!result ? result.error().message : ""));       \
    REQUIRE(result);                                     \
    std::move(result).value();                           \
  })
}
