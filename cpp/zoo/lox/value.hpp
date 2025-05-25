#pragma once

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

  class value_t {
    value_t() = default;

    value_t(value_t&&)                 = default;
    value_t(const value_t&)            = default;
    value_t& operator=(value_t&&)      = default;
    value_t& operator=(const value_t&) = default;

    template<typename T>
    explicit value_t(T&& data);

    friend class value_pool_t;

   public:
    variant_t<none_t, bool, double, string_t, function_t> data;

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

    friend expected_t<variant_t<double, string_t>> operator+(const value_t&, const value_t&);

    friend bool operator==(const value_t&, const value_t&);
    friend bool operator!=(const value_t&, const value_t&);

    friend string_or_view_t to_string_impl(const value_t&);
    friend std::ostream& operator<<(std::ostream&, const value_t&);
  };

  class value_ref_t;

  class value_pool_t : public menhir_t {
    struct item_t {
      index_t ref_count = 0;
      value_t value;
      index_t next_free = -1;
    };
    vector_t<item_t> items;
    index_t next_free = 0;

    friend class value_ref_t;

   public:
    template<typename T>
    value_ref_t make(T&&);
  };
  using value_pool_ptr_t = ptr_t<value_pool_t>;

  class value_ref_t {
    value_pool_ptr_t pool;
    index_t idx = 0;

    friend class value_pool_t;
    value_ref_t(value_pool_ptr_t, index_t);

   public:
    value_ref_t() = default;

    bool is_truthy() const;

    value_t* operator->() const;
    value_t& operator*() const;

    friend bool operator==(const value_ref_t&, const value_ref_t&);
    friend bool operator!=(const value_ref_t&, const value_ref_t&);

    friend string_or_view_t to_string_impl(const value_ref_t&);
    friend std::ostream& operator<<(std::ostream&, const value_ref_t&);
  };

  expected_t<value_ref_t> value_neg(value_pool_ptr_t, value_ref_t);
  expected_t<value_ref_t> value_inv(value_pool_ptr_t, value_ref_t);
  expected_t<value_ref_t> value_add(value_pool_ptr_t, value_ref_t, value_ref_t);
  expected_t<value_ref_t> value_sub(value_pool_ptr_t, value_ref_t, value_ref_t);
  expected_t<value_ref_t> value_mul(value_pool_ptr_t, value_ref_t, value_ref_t);
  expected_t<value_ref_t> value_div(value_pool_ptr_t, value_ref_t, value_ref_t);
  expected_t<value_ref_t> value_lt(value_pool_ptr_t, value_ref_t, value_ref_t);
  expected_t<value_ref_t> value_gt(value_pool_ptr_t, value_ref_t, value_ref_t);
  expected_t<value_ref_t> value_lte(value_pool_ptr_t, value_ref_t, value_ref_t);
  expected_t<value_ref_t> value_gte(value_pool_ptr_t, value_ref_t, value_ref_t);
  expected_t<value_ref_t> value_eq(value_pool_ptr_t, value_ref_t, value_ref_t);
  expected_t<value_ref_t> value_neq(value_pool_ptr_t, value_ref_t, value_ref_t);

  struct scope_t : public std::enable_shared_from_this<scope_t> {
    syntax_ward_ptr_t swp;
    scope_ptr_t parent;
    hashmap_t<token_id_t, value_ref_t> values;

    scope_t(syntax_ward_ptr_t, scope_ptr_t parent);

    expected_t<value_ref_t> get(token_id_t) const;

    // Assumes the name is already defined is some scope.
    expected_t<void> assign(token_id_t, value_ref_t);

    // Assumes the name is not defined yet in the local scope.
    expected_t<void> define(token_id_t, value_ref_t);

    scope_ptr_t make_child_scope();
  };
}

// IMPLEMENTATION

namespace silva::lox {
  template<typename T>
  value_t::value_t(T&& data) : data(std::forward<T>(data))
  {
  }

  template<typename T>
  value_ref_t value_pool_t::make(T&& x)
  {
    const index_t idx = items.size();
    items.push_back(item_t{
        .ref_count = 1,
        .value     = value_t{std::forward<T>(x)},
        .next_free = -1,
    });
    return value_ref_t{ptr(), idx};
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
