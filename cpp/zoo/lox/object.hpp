#pragma once

#include "cactus.hpp"
#include "object_pool.hpp"

#include "canopy/expected.hpp"

#include "syntax/parse_tree.hpp"

namespace silva::lox {

  struct object_t;
  using object_pool_t     = object_pool_t<object_t>;
  using object_pool_ptr_t = object_pool_ptr_t<object_t>;
  using object_ref_t      = object_ref_t<object_t>;

  using scope_ptr_t = cactus_arm_t<token_id_t, object_ref_t>;

  struct function_t {
    parse_tree_span_t pts;
    scope_ptr_t closure;

    index_t arity() const;
    parse_tree_span_t parameters() const;
    parse_tree_span_t body() const;

    friend bool operator==(const function_t&, const function_t&) = default;
  };

  struct function_builtin_t : public function_t {
    silva::function_t<object_ref_t(object_pool_t&, scope_ptr_t)> impl;

    friend bool operator==(const function_builtin_t& lhs, const function_builtin_t& rhs);
    friend bool operator!=(const function_builtin_t& lhs, const function_builtin_t& rhs) = default;
  };

  struct class_t {
    parse_tree_span_t pts;
    scope_ptr_t scope;

    friend bool operator==(const class_t&, const class_t&) = default;
  };

  struct class_instance_t {
    object_ref_t _class;
    scope_ptr_t scope;

    friend bool operator==(const class_instance_t&, const class_instance_t&) = default;
  };

  expected_t<object_ref_t> member_access(const object_ref_t& class_instance,
                                         token_id_t field_name,
                                         bool create_if_nonexistent,
                                         object_pool_t& pool,
                                         const token_id_t ti_this);

  struct object_t {
    object_t() = default;

    template<typename T>
    explicit object_t(T&& data);

    object_t(object_t&&)                 = default;
    object_t(const object_t&)            = default;
    object_t& operator=(object_t&&)      = default;
    object_t& operator=(const object_t&) = default;

    variant_t<none_t,
              bool,
              double,
              string_t,
              function_t,
              function_builtin_t,
              class_t,
              class_instance_t>
        data;

    bool is_none() const;
    bool holds_bool() const;
    bool holds_double() const;
    bool holds_string() const;

    bool holds_fundamental() const;

    bool holds_function() const;
    bool holds_function_builtin() const;
    bool holds_class() const;
    bool holds_class_instance() const;

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
