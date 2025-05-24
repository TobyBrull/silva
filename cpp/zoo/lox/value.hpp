#pragma once

#include "canopy/expected.hpp"

#include "syntax/parse_tree.hpp"

namespace silva::lox {

  struct function_t {
    parse_tree_span_t pts;

    index_t arity() const;
    parse_tree_span_t parameters() const;
    parse_tree_span_t body() const;

    friend bool operator==(const function_t&, const function_t&) = default;
  };

  struct value_t {
    variant_t<none_t, bool, double, string_t, function_t> data;

    value_t() = default;

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
    bool holds_function() const;

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

  struct scope_t;
  using scope_ptr_t = shared_ptr_t<scope_t>;
  struct scope_t : public std::enable_shared_from_this<scope_t> {
    syntax_ward_ptr_t swp;
    scope_ptr_t parent;
    hashmap_t<token_id_t, value_t> values;

    expected_t<const value_t*> get(token_id_t) const;

    // Assumes the name is already defined is some scope.
    expected_t<void> assign(token_id_t, value_t);

    // Assumes the name is not defined yet in the local scope.
    expected_t<void> define(token_id_t, value_t);

    scope_ptr_t make_child_scope();
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
