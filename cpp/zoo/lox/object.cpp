#include "object.hpp"

namespace silva::lox {

  // function_t

  index_t function_t::arity() const
  {
    return pts[1].num_children;
  }
  parse_tree_span_t function_t::parameters() const
  {
    return pts.sub_tree_span_at(1);
  }
  parse_tree_span_t function_t::body() const
  {
    return pts.sub_tree_span_at(pts[1].subtree_size + 1);
  }

  // class_instance_t

  class_t& class_instance_t::get_class() const
  {
    SILVA_ASSERT(_class->holds_class());
    return std::get<class_t>(_class->data);
  }

  expected_t<object_ref_t> class_instance_t::member_access(object_pool_t& pool,
                                                           const token_id_t ti_this,
                                                           const object_ref_t& ref_this,
                                                           const token_id_t field_name,
                                                           const bool create_if_nonexistent)
  {
    if (const auto it = fields.find(field_name); it != fields.end()) {
      return it->second;
    }
    const auto& cc = get_class();
    if (const auto it = cc.methods.find(field_name); it != cc.methods.end()) {
      const object_ref_t method = it->second;
      SILVA_EXPECT(method->holds_function(), ASSERT);
      const function_t& fun = std::get<function_t>(method->data);
      auto bound_scope      = SILVA_EXPECT_FWD(fun.closure.define(ti_this, ref_this));
      function_t bound_func{
          .pts     = fun.pts,
          .closure = std::move(bound_scope),
      };
      return pool.make(std::move(bound_func));
    }
    SILVA_EXPECT(create_if_nonexistent, MINOR, "couldn't access member");
    return fields[field_name] = pool.make(none);
  }

  // object_t

  bool object_t::is_none() const
  {
    return variant_holds_t<none_t>{}(data);
  }
  bool object_t::holds_bool() const
  {
    return variant_holds_t<bool>{}(data);
  }
  bool object_t::holds_double() const
  {
    return variant_holds_t<double>{}(data);
  }
  bool object_t::holds_string() const
  {
    return variant_holds_t<string_t>{}(data);
  }
  bool object_t::holds_function() const
  {
    return variant_holds_t<function_t>{}(data);
  }
  bool object_t::holds_class() const
  {
    return variant_holds_t<class_t>{}(data);
  }
  bool object_t::holds_class_instance() const
  {
    return variant_holds_t<class_instance_t>{}(data);
  }

  bool object_t::is_truthy() const
  {
    if (is_none()) {
      return false;
    }
    if (holds_bool()) {
      return std::get<bool>(data);
    }
    return true;
  }

  bool operator!(const object_t& x)
  {
    return !x.is_truthy();
  }
  expected_t<double> operator-(const object_t& x)
  {
    if (x.holds_double()) {
      return -std::get<double>(x.data);
    }
    else {
      SILVA_EXPECT(false, MAJOR, "runtime type error: - {} ", to_string(x));
    }
  }

#define BINARY_DOUBLE(return_type, op)                                            \
  expected_t<return_type> operator op(const object_t & lhs, const object_t & rhs) \
  {                                                                               \
    if (lhs.holds_double() && rhs.holds_double()) {                               \
      return std::get<double>(lhs.data) op std::get<double>(rhs.data);            \
    }                                                                             \
    else {                                                                        \
      SILVA_EXPECT(false,                                                         \
                   MAJOR,                                                         \
                   "runtime type error: {} " #op " {}",                           \
                   to_string(lhs),                                                \
                   to_string(rhs));                                               \
    }                                                                             \
  }
  BINARY_DOUBLE(double, *)
  BINARY_DOUBLE(double, /)
  BINARY_DOUBLE(double, -)
  BINARY_DOUBLE(bool, <)
  BINARY_DOUBLE(bool, >)
  BINARY_DOUBLE(bool, <=)
  BINARY_DOUBLE(bool, >=)
#undef BINARY_DOUBLE

  expected_t<variant_t<double, string_t>> operator+(const object_t& lhs, const object_t& rhs)
  {
    if (lhs.holds_double() && rhs.holds_double()) {
      return std::get<double>(lhs.data) + std::get<double>(rhs.data);
    }
    else if (lhs.holds_string() && rhs.holds_string()) {
      return std::get<string_t>(lhs.data) + std::get<string_t>(rhs.data);
    }
    else {
      SILVA_EXPECT(false, MAJOR, "runtime type error: {} + {}", to_string(lhs), to_string(rhs));
    }
  }

  struct equal_visitor_t {
    template<typename T>
    bool operator()(const T& ll, const T& rr) const
    {
      return ll == rr;
    }
    bool operator()(const auto& ll, const auto& rr) const { return false; }
  };
  bool operator==(const object_t& lhs, const object_t& rhs)
  {
    return std::visit(equal_visitor_t{}, lhs.data, rhs.data);
  }
  bool operator!=(const object_t& lhs, const object_t& rhs)
  {
    return !(lhs == rhs);
  }

  struct object_to_string_impl_visitor_t {
    string_or_view_t operator()(const none_t& x) const
    {
      return string_or_view_t{string_view_t{"none"}};
    }
    string_or_view_t operator()(const bool& x) const
    {
      if (x) {
        return string_or_view_t{string_view_t{"true"}};
      }
      else {
        return string_or_view_t{string_view_t{"false"}};
      }
    }
    string_or_view_t operator()(const double& x) const
    {
      auto retval = std::to_string(x);
      while (retval.size() >= 2 && retval.back() == '0') {
        retval.pop_back();
      }
      if (retval.size() >= 2 && retval.back() == '.') {
        retval.pop_back();
      }
      return string_or_view_t{std::move(retval)};
    }
    string_or_view_t operator()(const string_t& x) const { return string_or_view_t{string_t{x}}; }
    string_or_view_t operator()(const function_t& x) const
    {
      return string_or_view_t{fmt::format("<function {}>", to_string(x.pts))};
    }
    string_or_view_t operator()(const class_t& x) const
    {
      return string_or_view_t{fmt::format("<class {}>", to_string(x.pts))};
    }
    string_or_view_t operator()(const class_instance_t& x) const
    {
      return string_or_view_t{fmt::format("<instance of {}>", to_string(x._class))};
    }
    string_or_view_t operator()(const auto& x) const
    {
      return string_or_view_t{string_view_t{"Unknown lox::object_t"}};
    }
  };
  string_or_view_t to_string_impl(const object_t& value)
  {
    return std::visit(object_to_string_impl_visitor_t{}, value.data);
  }
  std::ostream& operator<<(std::ostream& os, const object_t& x)
  {
    return os << to_string_impl(x).as_string_view();
  }

  expected_t<object_ref_t> neg(object_pool_t& pool, object_ref_t x)
  {
    return pool.make(!(*x));
  }
  expected_t<object_ref_t> inv(object_pool_t& pool, object_ref_t x)
  {
    return pool.make(SILVA_EXPECT_FWD(-(*x)));
  }
  expected_t<object_ref_t> add(object_pool_t& pool, object_ref_t lhs, object_ref_t rhs)
  {
    auto res = SILVA_EXPECT_FWD(*lhs + *rhs);
    return std::visit([&](auto x) { return pool.make(std::move(x)); }, std::move(res));
  }
#define OBJECT_IMPL(func_name, op)                                                            \
  expected_t<object_ref_t> func_name(object_pool_t& pool, object_ref_t lhs, object_ref_t rhs) \
  {                                                                                           \
    return pool.make(SILVA_EXPECT_FWD(*lhs op * rhs));                                        \
  }
  OBJECT_IMPL(sub, -);
  OBJECT_IMPL(mul, *);
  OBJECT_IMPL(div, /);
  OBJECT_IMPL(lt, <);
  OBJECT_IMPL(gt, >);
  OBJECT_IMPL(lte, <=);
  OBJECT_IMPL(gte, >=);
#undef OBJECT_IMPL
  expected_t<object_ref_t> eq(object_pool_t& pool, object_ref_t lhs, object_ref_t rhs)
  {
    return pool.make(*lhs == *rhs);
  }
  expected_t<object_ref_t> neq(object_pool_t& pool, object_ref_t lhs, object_ref_t rhs)
  {
    return pool.make(*lhs != *rhs);
  }
}
