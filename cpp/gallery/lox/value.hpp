#pragma once

#include "canopy/expected.hpp"

namespace silva::lox {
  struct value_t {
    variant_t<none_t, bool, double, string_t> data;

    value_t(value_t&&)                 = default;
    value_t(const value_t&)            = default;
    value_t& operator=(value_t&&)      = default;
    value_t& operator=(const value_t&) = default;

    template<typename T>
    explicit value_t(T&& data);

    bool is_none() const;
    bool holds_bool() const;
    bool holds_double() const;
    bool holds_string() const;

    bool is_truthy() const;

    friend bool operator!(const value_t&);
    friend expected_t<double> operator-(const value_t&);

    friend expected_t<double> operator*(const value_t&, const value_t&);
    friend expected_t<double> operator/(const value_t&, const value_t&);
    friend expected_t<double> operator-(const value_t&, const value_t&);

    friend expected_t<bool> operator<(const value_t&, const value_t&);
    friend expected_t<bool> operator>(const value_t&, const value_t&);
    friend expected_t<bool> operator<=(const value_t&, const value_t&);
    friend expected_t<bool> operator>=(const value_t&, const value_t&);

    friend expected_t<value_t> operator+(const value_t&, const value_t&);

    friend bool operator==(const value_t&, const value_t&);
    friend bool operator!=(const value_t&, const value_t&);

    friend string_or_view_t to_string_impl(const value_t&);
    friend std::ostream& operator<<(std::ostream&, const value_t&);
  };
}

// IMPLEMENTATION

namespace silva::lox {
  template<typename T>
  value_t::value_t(T&& data) : data(std::forward<T>(data))
  {
  }
}

template<>
struct fmt::formatter<silva::lox::value_t> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(const silva::lox::value_t& x, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "lox_v[{}]", to_string_impl(x));
  }
};
