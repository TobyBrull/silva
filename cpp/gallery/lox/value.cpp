#include "value.hpp"

namespace silva::lox {
  bool value_t::is_none() const
  {
    return variant_holds_t<std::nullopt_t>{}(data);
  }
  bool value_t::holds_bool() const
  {
    return variant_holds_t<bool>{}(data);
  }
  bool value_t::holds_double() const
  {
    return variant_holds_t<double>{}(data);
  }
  bool value_t::holds_string() const
  {
    return variant_holds_t<string_t>{}(data);
  }

  bool value_t::is_truthy() const
  {
    if (is_none()) {
      return false;
    }
    if (holds_bool()) {
      return std::get<bool>(data);
    }
    return true;
  }

  expected_t<value_t> operator!(const value_t& x)
  {
    return !x.is_truthy();
  }
  expected_t<value_t> operator-(const value_t& x)
  {
    if (x.holds_double()) {
      return std::get<double>(x.data);
    }
    else {
      SILVA_EXPECT(false, MAJOR, "runtime type error: - {} ", to_string(x));
    }
  }

#define BINARY_DOUBLE(op)                                                   \
  expected_t<value_t> operator op(const value_t & lhs, const value_t & rhs) \
  {                                                                         \
    if (lhs.holds_double() && rhs.holds_double()) {                         \
      return {std::get<double>(lhs.data) op std::get<double>(rhs.data)};    \
    }                                                                       \
    else {                                                                  \
      SILVA_EXPECT(false,                                                   \
                   MAJOR,                                                   \
                   "runtime type error: {} " #op " {}",                     \
                   to_string(lhs),                                          \
                   to_string(rhs));                                         \
    }                                                                       \
  }
  BINARY_DOUBLE(*)
  BINARY_DOUBLE(/)
  BINARY_DOUBLE(+)
  BINARY_DOUBLE(-)
#undef BINARY_DOUBLE

  struct value_to_string_impl_visitor_t {
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
    string_or_view_t operator()(const auto& x) const
    {
      return string_or_view_t{string_view_t{"Unknown lox::value_t"}};
    }
  };
  string_or_view_t to_string_impl(const value_t& value)
  {
    return std::visit(value_to_string_impl_visitor_t{}, value.data);
  }
}
