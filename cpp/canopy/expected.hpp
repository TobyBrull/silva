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

  // If std::remove_cv_t<T> already is_expected_t, returns "x" as is.
  // Otherwise, wraps "x" in an expected_t.
  template<typename T>
  auto expectify(T&& x);

  struct expected_traits_t {
    bool materialize_fwd = false;
  };
  constexpr inline expected_traits_t expected_traits;

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
#define SILVA_EXPECT_IMPL(return_stmt, condition, error_level, ...)                   \
  do {                                                                                \
    using enum silva::error_level_t;                                                  \
    static_assert(silva::error_level_is_primary(error_level));                        \
    if (!(condition)) {                                                               \
      return_stmt std::unexpected(silva::impl::silva_expect(__FILE__,                 \
                                                            __LINE__,                 \
                                                            error_level,              \
                                                            #condition __VA_OPT__(, ) \
                                                                __VA_ARGS__));        \
    }                                                                                 \
  } while (false)
#define SILVA_EXPECT(...) SILVA_EXPECT_IMPL(return, __VA_ARGS__)

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
#define SILVA_EXPECT_FWD_IMPL(return_stmt, return_stmt_2, expression, ...)                         \
  ({                                                                                               \
    auto __silva_result = (expression);                                                            \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value);                          \
    if (!__silva_result.has_value()) {                                                             \
      if constexpr (expected_traits.materialize_fwd) {                                             \
        __silva_result.error().materialize();                                                      \
      }                                                                                            \
      using enum error_level_t;                                                                    \
      return_stmt std::unexpected(silva::impl::silva_expect_fwd(__FILE__,                          \
                                                                __LINE__,                          \
                                                                std::move(__silva_result).error(), \
                                                                #expression __VA_OPT__(, )         \
                                                                    __VA_ARGS__));                 \
      return_stmt_2;                                                                               \
    }                                                                                              \
    *std::move(__silva_result);                                                                    \
  })
#define SILVA_EXPECT_FWD(...) SILVA_EXPECT_FWD_IMPL(return, , __VA_ARGS__)

#define SILVA_EXPECT_FWD_PLAIN(expression)                                \
  ({                                                                      \
    auto __silva_result = (expression);                                   \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value); \
    if (!__silva_result.has_value()) {                                    \
      if constexpr (expected_traits.materialize_fwd) {                    \
        __silva_result.error().materialize();                             \
      }                                                                   \
      return std::unexpected(std::move(__silva_result).error());          \
    }                                                                     \
    *std::move(__silva_result);                                           \
  })

// Semantics:
// Like SILVA_EXPECT_FWD, but replaces the forwarded error message with the one given to this macro.
//
// Usage: Like SILVA_EXPECT_FWD()
#define SILVA_EXPECT_FWD_AS(expression, ...)                              \
  ({                                                                      \
    auto __silva_result = (expression);                                   \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value); \
    if (!__silva_result.has_value()) {                                    \
      auto error = std::move(__silva_result).error();                     \
      using enum error_level_t;                                           \
      silva::impl::silva_expect_fwd_as(error __VA_OPT__(, ) __VA_ARGS__); \
      if constexpr (expected_traits.materialize_fwd) {                    \
        error.materialize();                                              \
      }                                                                   \
      return std::unexpected(std::move(error));                           \
    }                                                                     \
    *std::move(__silva_result);                                           \
  })

// Semantics:
// Must only be used inside functions whose return value is "expected_t<...>". Also the provided
// "expression" must evaluate to a type of the form "expected_t<Result>". If the result of
// "expression" contains an error with level greater or equal to the provided "error_level", this
// causes a return from the function with the same error-level. Otherwise, the expression
// represented by this macro evaluates to the result of "expression".
//
// Usage:
//  - SILVA_EXPECT_FWD_IF(MAJOR, foo(x))
//
#define SILVA_EXPECT_FWD_IF(error_level, expression)                                    \
  ({                                                                                    \
    auto __silva_result = (expression);                                                 \
    using enum error_level_t;                                                           \
    static_assert(error_level_is_primary(error_level));                                 \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value);               \
    if (!__silva_result.has_value() && (__silva_result.error().level >= error_level)) { \
      if constexpr (expected_traits.materialize_fwd) {                                  \
        __silva_result.error().materialize();                                           \
      }                                                                                 \
      return std::unexpected(std::move(__silva_result).error());                        \
    }                                                                                   \
    std::move(__silva_result);                                                          \
  })

#define SILVA_EXPECT_ASSERT(x)                                                                 \
  ({                                                                                           \
    auto result = (x);                                                                         \
    static_assert(is_expected_t<decltype(result)>::value);                                     \
    SILVA_ASSERT(result.has_value(), "Unexpected:\n{}", silva::pretty_string(result.error())); \
    *std::move(result);                                                                        \
  })

// Semantics:
// For Catch2. The provided "expression" must evaluate to a type of the form "expected_t<Result>".
// If the result of "expression" contains an error, a Catch2 "REQUIRE" error is generated (which
// stops further execution of the unit-test). Otherwise, the expression represented by this macro
// evaluates to the contained value of type "Result".
//
// Usage:
//  - SILVA_REQUIRE(foo(x))
//
#define SILVA_REQUIRE(expression)                                                            \
  ({                                                                                         \
    auto __silva_result = (expression);                                                      \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value);                    \
    INFO((!__silva_result.has_value() ? silva::pretty_string(__silva_result.error()) : "")); \
    REQUIRE(__silva_result);                                                                 \
    *std::move(__silva_result);                                                              \
  })
}
#define SILVA_REQUIRE_ERROR(expression)                                   \
  ({                                                                      \
    auto __silva_result = (expression);                                   \
    static_assert(silva::is_expected_t<decltype(__silva_result)>::value); \
    REQUIRE(!__silva_result.has_value());                                 \
    auto err_msg = __silva_result.error().to_string_plain().as_string();  \
    std::move(err_msg);                                                   \
  })

// IMPLEMENTATION

namespace silva {
  template<typename T>
  auto expectify(T&& x)
  {
    using TT = std::remove_cv_t<T>;
    if constexpr (is_expected_t<TT>::value) {
      return std::forward<T>(x);
    }
    else {
      return expected_t<TT>{std::forward<T>(x)};
    }
  }
}

namespace silva::impl {
  constexpr index_t max_expr_str_len = 30;

  inline error_t silva_expect(char const* file,
                              const long line,
                              const error_level_t error_level,
                              char const* expr_str)
  {
    const string_view_t expr_sv{expr_str};
    if (expr_sv.size() > max_expr_str_len) {
      return make_error(error_level,
                        {},
                        "unexpected [{}...] at [{}:{}]",
                        expr_sv.substr(0, max_expr_str_len),
                        string_view_t{file},
                        line);
    }
    else {
      return make_error(error_level,
                        {},
                        "unexpected [{}] at [{}:{}]",
                        expr_sv,
                        string_view_t{file},
                        line);
    }
  }

  template<typename... Args>
    requires(sizeof...(Args) >= 1)
  error_t silva_expect(char const* file,
                       const long line,
                       const error_level_t error_level,
                       char const* expr_str,
                       Args&&... args)
  {
    return make_error(error_level, {}, std::forward<Args>(args)...);
  }

  template<typename... Args>
  error_t silva_expect_fwd(char const* file,
                           const long line,
                           error_t error,
                           char const* expr_str,
                           const error_level_t error_level,
                           Args&&... args)
  {
    const error_level_t new_error_level = std::max(error.level, error_level);
    std::array<error_t, 1> error_array{std::move(error)};
    if constexpr (sizeof...(Args) >= 1) {
      error = make_error(new_error_level, error_array, std::forward<Args>(args)...);
    }
    else {
      const string_view_t expr_sv{expr_str};
      if (expr_sv.size() > max_expr_str_len) {
        error = make_error(new_error_level,
                           error_array,
                           "while calling [{}...] at [{}:{}]",
                           expr_sv.substr(0, max_expr_str_len),
                           string_view_t{file},
                           line);
      }
      else {
        error = make_error(new_error_level,
                           error_array,
                           "while calling [{}] at [{}:{}]",
                           expr_sv,
                           string_view_t{file},
                           line);
      }
    }
    return std::move(error);
  }

  template<typename... Args>
  error_t silva_expect_fwd(char const* file,
                           const long line,
                           error_t error,
                           char const* expr_str,
                           Args&&... args)
  {
    return silva_expect_fwd(file,
                            line,
                            std::move(error),
                            expr_str,
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
