#pragma once

#include "cactus.hpp"
#include "lox.hpp"
#include "object_pool.hpp"

#include "canopy/expected.hpp"

#include "syntax/parse_tree.hpp"

namespace silva::lox {

  struct object_t;
  struct object_pool_t;
  using object_pool_ptr_t = object_pool_ptr_t<object_t>;
  using object_ref_t      = object_ref_t<object_t>;

  using scope_ptr_t = cactus_arm_t<token_id_t, object_ref_t>;

  struct function_t {
    parse_tree_span_t pts;
    scope_ptr_t closure;

    index_t arity() const;
    parse_tree_span_t parameters() const;
    parse_tree_span_t body() const;

    friend bool operator==(const function_t&, const function_t&);
  };

  struct function_builtin_t : public function_t {
    silva::function_t<object_ref_t(object_pool_t&, scope_ptr_t)> impl;

    friend bool operator==(const function_builtin_t&, const function_builtin_t&);
  };

  struct class_t {
    parse_tree_span_t pts;
    object_ref_t superclass;
    hash_map_t<token_id_t, object_ref_t> methods;

    friend bool operator==(const class_t&, const class_t&);
  };

  struct class_instance_t {
    object_ref_t _class;
    hash_map_t<token_id_t, object_ref_t> fields;

    friend bool operator==(const class_instance_t&, const class_instance_t&);
  };

  expected_t<object_ref_t> member_get(const object_ref_t& class_instance,
                                      token_id_t field_name,
                                      object_pool_t& pool,
                                      const token_id_t ti_this);
  expected_t<object_ref_t> member_bind(const object_ref_t& class_instance,
                                       object_ref_t _class,
                                       token_id_t field_name,
                                       object_pool_t& pool,
                                       const token_id_t ti_this);

  expected_t<object_ref_t>
  object_ref_from_literal(const parse_tree_span_t&, object_pool_t&, const lexicon_t&);

  struct object_t {
    object_t() = default;

    template<typename T>
    explicit object_t(T&& data);

    object_t(object_t&&)                 = default;
    object_t(const object_t&)            = default;
    object_t& operator=(object_t&&)      = delete;
    object_t& operator=(const object_t&) = delete;

    variant_t<const none_t,
              const bool,
              const double,
              const string_t,
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

    expected_t<double> as_double() const;
    expected_t<string_t> as_string() const;

    void clear_scopes();

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

    friend void pretty_write_impl(const object_t&, byte_sink_t*);
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

  struct object_pool_t : public silva::object_pool_t<object_t> {
    using parent_t = silva::object_pool_t<object_t>;

    object_pool_t();

    object_ref_t const_nil   = parent_t::make(none);
    object_ref_t const_true  = parent_t::make(true);
    object_ref_t const_false = parent_t::make(false);

    template<typename Arg>
    object_ref_t make(Arg&&);
  };
}

// IMPLEMENTATION

namespace silva::lox {
  template<typename T>
  object_t::object_t(T&& data) : data(std::forward<T>(data))
  {
  }

  inline object_pool_t::object_pool_t() {}

  template<typename Arg>
  object_ref_t object_pool_t::make(Arg&& arg)
  {
    if constexpr (std::same_as<Arg, none_t>) {
      return const_nil;
    }
    else if constexpr (std::same_as<Arg, bool>) {
      if (arg == true) {
        return const_true;
      }
      else {
        return const_false;
      }
    }
    else {
      return parent_t::make(std::forward<Arg>(arg));
    }
  }
}

template<>
struct fmt::formatter<silva::lox::object_t> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(const silva::lox::object_t& x, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "lox_v[{}]", silva::pretty_string(x));
  }
};
