#include "dyn_object.hpp"

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

  // dyn_object_t

  bool dyn_object_t::is_none() const
  {
    return variant_holds_t<none_t>{}(data);
  }
  bool dyn_object_t::holds_bool() const
  {
    return variant_holds_t<bool>{}(data);
  }
  bool dyn_object_t::holds_double() const
  {
    return variant_holds_t<double>{}(data);
  }
  bool dyn_object_t::holds_string() const
  {
    return variant_holds_t<string_t>{}(data);
  }
  bool dyn_object_t::holds_function() const
  {
    return variant_holds_t<function_t>{}(data);
  }

  bool dyn_object_t::is_truthy() const
  {
    if (is_none()) {
      return false;
    }
    if (holds_bool()) {
      return std::get<bool>(data);
    }
    return true;
  }

  bool operator!(const dyn_object_t& x)
  {
    return !x.is_truthy();
  }
  expected_t<double> operator-(const dyn_object_t& x)
  {
    if (x.holds_double()) {
      return -std::get<double>(x.data);
    }
    else {
      SILVA_EXPECT(false, MAJOR, "runtime type error: - {} ", to_string(x));
    }
  }

#define BINARY_DOUBLE(return_type, op)                                                    \
  expected_t<return_type> operator op(const dyn_object_t & lhs, const dyn_object_t & rhs) \
  {                                                                                       \
    if (lhs.holds_double() && rhs.holds_double()) {                                       \
      return std::get<double>(lhs.data) op std::get<double>(rhs.data);                    \
    }                                                                                     \
    else {                                                                                \
      SILVA_EXPECT(false,                                                                 \
                   MAJOR,                                                                 \
                   "runtime type error: {} " #op " {}",                                   \
                   to_string(lhs),                                                        \
                   to_string(rhs));                                                       \
    }                                                                                     \
  }
  BINARY_DOUBLE(double, *)
  BINARY_DOUBLE(double, /)
  BINARY_DOUBLE(double, -)
  BINARY_DOUBLE(bool, <)
  BINARY_DOUBLE(bool, >)
  BINARY_DOUBLE(bool, <=)
  BINARY_DOUBLE(bool, >=)
#undef BINARY_DOUBLE

  expected_t<variant_t<double, string_t>> operator+(const dyn_object_t& lhs,
                                                    const dyn_object_t& rhs)
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
  bool operator==(const dyn_object_t& lhs, const dyn_object_t& rhs)
  {
    return std::visit(equal_visitor_t{}, lhs.data, rhs.data);
  }
  bool operator!=(const dyn_object_t& lhs, const dyn_object_t& rhs)
  {
    return !(lhs == rhs);
  }

  struct dyn_object_to_string_impl_visitor_t {
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
    string_or_view_t operator()(const auto& x) const
    {
      return string_or_view_t{string_view_t{"Unknown lox::dyn_object_t"}};
    }
  };
  string_or_view_t to_string_impl(const dyn_object_t& value)
  {
    return std::visit(dyn_object_to_string_impl_visitor_t{}, value.data);
  }
  std::ostream& operator<<(std::ostream& os, const dyn_object_t& x)
  {
    return os << to_string_impl(x).as_string_view();
  }

  expected_t<dyn_object_ref_t> neg(dyn_object_pool_t& pool, dyn_object_ref_t x)
  {
    return pool.make(!(*x));
  }
  expected_t<dyn_object_ref_t> inv(dyn_object_pool_t& pool, dyn_object_ref_t x)
  {
    return pool.make(SILVA_EXPECT_FWD(-(*x)));
  }
  expected_t<dyn_object_ref_t>
  add(dyn_object_pool_t& pool, dyn_object_ref_t lhs, dyn_object_ref_t rhs)
  {
    auto res = SILVA_EXPECT_FWD(*lhs + *rhs);
    return std::visit([&](auto x) { return pool.make(std::move(x)); }, std::move(res));
  }
#define DYN_OBJECT_IMPL(func_name, op)                            \
  expected_t<dyn_object_ref_t> func_name(dyn_object_pool_t& pool, \
                                         dyn_object_ref_t lhs,    \
                                         dyn_object_ref_t rhs)    \
  {                                                               \
    return pool.make(SILVA_EXPECT_FWD(*lhs op * rhs));            \
  }
  DYN_OBJECT_IMPL(sub, -);
  DYN_OBJECT_IMPL(mul, *);
  DYN_OBJECT_IMPL(div, /);
  DYN_OBJECT_IMPL(lt, <);
  DYN_OBJECT_IMPL(gt, >);
  DYN_OBJECT_IMPL(lte, <=);
  DYN_OBJECT_IMPL(gte, >=);
#undef DYN_OBJECT_IMPL
  expected_t<dyn_object_ref_t>
  eq(dyn_object_pool_t& pool, dyn_object_ref_t lhs, dyn_object_ref_t rhs)
  {
    return pool.make(*lhs == *rhs);
  }
  expected_t<dyn_object_ref_t>
  neq(dyn_object_pool_t& pool, dyn_object_ref_t lhs, dyn_object_ref_t rhs)
  {
    return pool.make(*lhs != *rhs);
  }

  // scope_t

  scope_t::scope_t(syntax_ward_ptr_t swp, scope_ptr_t parent) : swp(swp), parent(std::move(parent))
  {
  }

  expected_t<dyn_object_ref_t> scope_t::get(const token_id_t ti) const
  {
    const scope_t* sp = this;
    while (true) {
      const auto it = sp->values.find(ti);
      if (it == sp->values.end()) {
        SILVA_EXPECT(sp->parent, MINOR, "couldn't find identifier {}", swp->token_id_wrap(ti));
        sp = sp->parent.get();
      }
      else {
        return {it->second};
      }
    }
  }
  expected_t<void> scope_t::assign(const token_id_t ti, dyn_object_ref_t x)
  {
    scope_t* sp = this;
    while (true) {
      auto it = sp->values.find(ti);
      if (it == sp->values.end()) {
        SILVA_EXPECT(sp->parent,
                     MINOR,
                     "couldn't find identifier {} (trying to assign {} to it)",
                     swp->token_id_wrap(ti),
                     to_string(x));
        sp = sp->parent.get();
      }
      else {
        it->second = std::move(x);
        return {};
      }
    }
  }
  expected_t<void> scope_t::define(const token_id_t ti, dyn_object_ref_t x)
  {
    const auto [it, inserted] = values.emplace(ti, dyn_object_ref_t{});
    SILVA_EXPECT(inserted,
                 MINOR,
                 "couldn't define identifier {} (with initializer {}) because the value already "
                 "exists in the local scope",
                 swp->token_id_wrap(ti),
                 to_string(x));
    it->second = std::move(x);
    return {};
  }
  scope_ptr_t scope_t::make_child_scope()
  {
    auto retval = std::make_shared<scope_t>(swp, shared_from_this());
    return retval;
  }
}
