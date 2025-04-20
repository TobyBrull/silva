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

  struct expected_traits_t {
    bool materialize_fwd = false;
  };
  inline expected_traits_t expected_traits;

// Semantics:
// Must only be used inside functions whose return value is "expected_t<...>". If the "condition"
// evaluates to false, causes a return from the function with an error whose message is specified by
// the optional arguments and with the given "error_level".
//
// Usage:
//  - SILVA_EXPECT(0 < x, MINOR);
//  - SILVA_EXPECT(0 < x, MINOR, "x too small");
//  - SILVA_EXPECT(0 < x, MINOR, "x (={}) must be positive", x);
//
#define SILVA_EXPECT(condition, error_level, ...)                                                 \
  do {                                                                                            \
    using enum error_level_t;                                                                     \
    static_assert(error_level_is_primary(error_level));                                           \
    if (!(condition)) {                                                                           \
      return std::unexpected(                                                                     \
          silva::impl::silva_expect(__FILE__, __LINE__, error_level __VA_OPT__(, ) __VA_ARGS__)); \
    }                                                                                             \
  } while (false)

// Semantics:
// Must only be used inside functions whose return value is "expected_t<...>". Also the provided
// "expression" must evaluate to a type of the form "expected_t<Result>". If the result of
// "expression" contains an error, this causes a return from the function with either the same
// error-level, or at least the error-level of the second argument if provided. Otherwise, the
// expression represented by this macro evaluates to the contained value of type "Result".
//
// Usage:
//  - SILVA_EXPECT_FWD(foo(x));
//  - SILVA_EXPECT_FWD(foo(x), "foo failed for x={}", x);
//    // Forward any error but raise the level to MAJOR if it's lower than MAJOR
//  - SILVA_EXPECT_FWD(foo(x), MAJOR)
//  - SILVA_EXPECT_FWD(foo(x), MAJOR, "foo failed for x={}", x);
//
#define SILVA_EXPECT_FWD(expression, ...)                                 \
  ({                                                                      \
    auto __silva_result = (expression);                                   \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value); \
    if (!__silva_result) {                                                \
      if (expected_traits.materialize_fwd) {                              \
        __silva_result.error().materialize();                             \
      }                                                                   \
      using enum error_level_t;                                           \
      return std::unexpected(silva::impl::silva_expect_fwd(               \
          __FILE__,                                                       \
          __LINE__,                                                       \
          std::move(__silva_result).error() __VA_OPT__(, ) __VA_ARGS__)); \
    }                                                                     \
    std::move(__silva_result).value();                                    \
  })

// Semantics:
// Like SILVA_EXPECT_FWD, but replaces the forwarded error message with the one given to this macro.
//
// Usage: Like SILVA_EXPECT_FWD()
#define SILVA_EXPECT_FWD_AS(expression, ...)                              \
  ({                                                                      \
    auto __silva_result = (expression);                                   \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value); \
    if (!__silva_result) {                                                \
      auto error = std::move(__silva_result).error();                     \
      using enum error_level_t;                                           \
      silva::impl::silva_expect_fwd_as(error __VA_OPT__(, ) __VA_ARGS__); \
      if (expected_traits.materialize_fwd) {                              \
        error.materialize();                                              \
      }                                                                   \
      return std::unexpected(std::move(error));                           \
    }                                                                     \
    std::move(__silva_result).value();                                    \
  })

// Semantics:
// Must only be used inside functions whose return value is "expected_t<...>". Also the provided
// "expression" must evaluate to a type of the form "expected_t<Result>". If the result of
// "expression" contains an error with level greater or equal to the provided "error_level", this
// causes a return from the function with the same error-level. Otherwise, the expression
// represented by this macro evaluates to the result of "expression".
//
// Usage:
//  - SILVA_EXPECT_FWD_IF(foo(x), MAJOR)
//
#define SILVA_EXPECT_FWD_IF(expression, error_level)                        \
  ({                                                                        \
    auto __silva_result = (expression);                                     \
    using enum error_level_t;                                               \
    static_assert(error_level_is_primary(error_level));                     \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value);   \
    if (!__silva_result && (__silva_result.error().level >= error_level)) { \
      if (expected_traits.materialize_fwd) {                                \
        __silva_result.error().materialize();                               \
      }                                                                     \
      return std::unexpected(std::move(__silva_result).error());            \
    }                                                                       \
    std::move(__silva_result);                                              \
  })

#define SILVA_EXPECT_ASSERT(x)                                          \
  ({                                                                    \
    auto result = (x);                                                  \
    static_assert(is_expected_t<decltype(result)>::value);              \
    SILVA_ASSERT(result, "Unexpected:\n{}", to_string(result.error())); \
    std::move(result).value();                                          \
  })

// Semantics:
// For Catch2. The provided "expression" must evaluate to a type of the form "expected_t<Result>".
// If the result of "expression" contains an error, a Catch2 "REQUIRE" error is generated (which
// stops further execution of the unit-test). Otherwise, the expression represented by this macro
// evaluates to the contained value of type "Result".
//
// Usage:
//  - SILVA_EXPECT_REQUIRE(foo(x))
//
#define SILVA_EXPECT_REQUIRE(expression)                                               \
  ({                                                                                   \
    auto __silva_result = (expression);                                                \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value);              \
    INFO((!__silva_result ? to_string(__silva_result.error()).as_string_view() : "")); \
    REQUIRE(__silva_result);                                                           \
    std::move(__silva_result).value();                                                 \
  })
}

// IMPLEMENTATION

namespace silva::impl {
  inline error_t silva_expect(char const* file, const long line, const error_level_t error_level)
  {
    return make_error(error_level,
                      {},
                      "unexpected condition in [{}:{}]",
                      string_view_t{file},
                      line);
  }

  template<typename... Args>
    requires(sizeof...(Args) >= 1)
  error_t
  silva_expect(char const* file, const long line, const error_level_t error_level, Args&&... args)
  {
    return make_error(error_level, {}, std::forward<Args>(args)...);
  }

  template<typename... Args>
  error_t silva_expect_fwd(char const* file,
                           const long line,
                           error_t error,
                           const error_level_t error_level,
                           Args&&... args)
  {
    const error_level_t new_error_level = std::max(error.level, error_level);
    std::array<error_t, 1> error_array{std::move(error)};
    if constexpr (sizeof...(Args) >= 1) {
      error = make_error(new_error_level, error_array, std::forward<Args>(args)...);
    }
    else {
      error = make_error(new_error_level,
                         error_array,
                         "unexpected forwareded at [{}:{}]",
                         string_view_t{file},
                         line);
    }
    return std::move(error);
  }

  template<typename... Args>
  error_t silva_expect_fwd(char const* file, const long line, error_t error, Args&&... args)
  {
    return silva_expect_fwd(file,
                            line,
                            std::move(error),
                            error_level_t::NO_ERROR,
                            std::forward<Args>(args)...);
  }

  template<typename... Args>
  void silva_expect_fwd_as(error_t& error, const error_level_t error_level, Args&&... args)
  {
    if (error_level != error_level_t::NO_ERROR) {
      error.level = error_level;
    }
    error.replace_message(std::forward<Args>(args)...);
  }

  template<typename... Args>
  void silva_expect_fwd_as(error_t& error, Args&&... args)
  {
    silva_expect_fwd_as(error, error_level_t::NO_ERROR, std::forward<Args>(args)...);
  }
}
