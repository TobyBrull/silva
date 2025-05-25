#pragma once

#include "object_pool.hpp"

#include "canopy/expected.hpp"

#include "syntax/parse_tree.hpp"

namespace silva::lox {

  struct scope_t;
  using scope_ptr_t = shared_ptr_t<scope_t>;

  struct function_t {
    parse_tree_span_t pts;
    scope_ptr_t closure;

    index_t arity() const;
    parse_tree_span_t parameters() const;
    parse_tree_span_t body() const;

    friend bool operator==(const function_t&, const function_t&) = default;
  };

  struct object_t {
    object_t() = default;

    template<typename T>
    explicit object_t(T&& data);

    object_t(object_t&&)                 = default;
    object_t(const object_t&)            = default;
    object_t& operator=(object_t&&)      = default;
    object_t& operator=(const object_t&) = default;

    variant_t<none_t, bool, double, string_t, function_t> data;

    bool is_none() const;
    bool holds_bool() const;
    bool holds_double() const;
    bool holds_string() const;
    bool holds_function() const;

    bool is_truthy() const;

    friend bool operator!(const object_t&);
    friend expected_t<double> operator-(const object_t&);

    friend expected_t<double> operator*(const object_t&, const object_t&);
    friend expected_t<double> operator/(const object_t&, const object_t&);
    friend expected_t<double> operator-(const object_t&, const object_t&);

    friend expected_t<bool> operator<(const object_t&, const object_t&);
    friend expected_t<bool> operator>(const object_t&, const object_t&);
    friend expected_t<bool> operator<=(const object_t&, const object_t&);
    friend expected_t<bool> operator>=(const object_t&, const object_t&);

    friend expected_t<variant_t<double, string_t>> operator+(const object_t&, const object_t&);

    friend bool operator==(const object_t&, const object_t&);
    friend bool operator!=(const object_t&, const object_t&);

    friend string_or_view_t to_string_impl(const object_t&);
    friend std::ostream& operator<<(std::ostream&, const object_t&);
  };

  using object_pool_t     = object_pool_t<object_t>;
  using object_pool_ptr_t = object_pool_ptr_t<object_t>;
  using object_ref_t      = object_ref_t<object_t>;

  expected_t<object_ref_t> neg(object_pool_t&, object_ref_t);
  expected_t<object_ref_t> inv(object_pool_t&, object_ref_t);
  expected_t<object_ref_t> add(object_pool_t&, object_ref_t, object_ref_t);
  expected_t<object_ref_t> sub(object_pool_t&, object_ref_t, object_ref_t);
  expected_t<object_ref_t> mul(object_pool_t&, object_ref_t, object_ref_t);
  expected_t<object_ref_t> div(object_pool_t&, object_ref_t, object_ref_t);
  expected_t<object_ref_t> lt(object_pool_t&, object_ref_t, object_ref_t);
  expected_t<object_ref_t> gt(object_pool_t&, object_ref_t, object_ref_t);
  expected_t<object_ref_t> lte(object_pool_t&, object_ref_t, object_ref_t);
  expected_t<object_ref_t> gte(object_pool_t&, object_ref_t, object_ref_t);
  expected_t<object_ref_t> eq(object_pool_t&, object_ref_t, object_ref_t);
  expected_t<object_ref_t> neq(object_pool_t&, object_ref_t, object_ref_t);

  struct scope_t : public std::enable_shared_from_this<scope_t> {
    syntax_ward_ptr_t swp;
    scope_ptr_t parent;
    hashmap_t<token_id_t, object_ref_t> values;

    scope_t(syntax_ward_ptr_t, scope_ptr_t parent);

    expected_t<object_ref_t> get(token_id_t) const;

    // Assumes the name is already defined is some scope.
    expected_t<void> assign(token_id_t, object_ref_t);

    // Assumes the name is not defined yet in the local scope.
    expected_t<void> define(token_id_t, object_ref_t);

    scope_ptr_t make_child_scope();
  };
}

// IMPLEMENTATION

namespace silva::lox {
  template<typename T>
  object_t::object_t(T&& data) : data(std::forward<T>(data))
  {
  }
}

template<>
struct fmt::formatter<silva::lox::object_t> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(const silva::lox::object_t& x, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "lox_v[{}]", to_string_impl(x));
  }
};
