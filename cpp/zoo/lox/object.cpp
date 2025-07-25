#include "object.hpp"

#include "bytecode.hpp"

namespace silva::lox {

  // functions

  function_t::function_t(parse_tree_span_t pts) : pts(std::move(pts)) {}
  function_t::function_t(function_t&&)            = default;
  function_t& function_t::operator=(function_t&&) = default;
  function_t::~function_t()                       = default;

  index_t function_t::arity() const
  {
    const index_t arity = pts[1].num_children;
    return arity;
  }
  parse_tree_span_t function_t::parameters() const
  {
    return pts.sub_tree_span_at(1);
  }
  parse_tree_span_t function_t::body() const
  {
    return pts.sub_tree_span_at(pts[1].subtree_size + 1);
  }

  bool operator==(const function_t& lhs, const function_t& rhs)
  {
    return &lhs == &rhs;
  }
  bool operator==(const function_builtin_t& lhs, const function_builtin_t& rhs)
  {
    return ((const function_t&)lhs) == ((const function_t&)rhs);
  }
  bool operator==(const class_t& lhs, const class_t& rhs)
  {
    return &lhs == &rhs;
  }
  bool operator==(const class_instance_t& lhs, const class_instance_t& rhs)
  {
    return &lhs == &rhs;
  }

  // class_instance_t

  expected_t<object_ref_t> member_get(const object_ref_t& class_instance,
                                      token_id_t field_name,
                                      object_pool_t& object_pool,
                                      const token_id_t ti_this)
  {
    SILVA_EXPECT(class_instance->holds_class_instance(),
                 MINOR,
                 "can only get member from class instance, not from {}",
                 pretty_string(class_instance));
    class_instance_t& ci = std::get<class_instance_t>(class_instance->data);
    if (const auto it = ci.fields.find(field_name); it != ci.fields.end()) {
      return it->second;
    }
    return member_bind(class_instance, {}, field_name, object_pool, ti_this);
  }

  expected_t<object_ref_t> member_bind(const object_ref_t& class_instance,
                                       object_ref_t _class,
                                       token_id_t field_name,
                                       object_pool_t& object_pool,
                                       const token_id_t ti_this)
  {
    SILVA_EXPECT(class_instance->holds_class_instance(),
                 MINOR,
                 "can only get member from class instance",
                 pretty_string(class_instance));
    class_instance_t& ci = std::get<class_instance_t>(class_instance->data);
    if (_class.is_nullptr()) {
      _class = ci._class;
    }
    while (!_class.is_nullptr()) {
      SILVA_EXPECT(_class->holds_class(), ASSERT);
      const class_t& ccc = std::get<class_t>(_class->data);
      const auto it      = ccc.methods.find(field_name);
      if (it == ccc.methods.end()) {
        _class = ccc.superclass;
        continue;
      }
      else {
        const object_ref_t method = it->second;
        SILVA_EXPECT(method->holds_function(), ASSERT);
        const function_t& fun  = std::get<function_t>(method->data);
        auto closure_with_this = fun.closure.make_child_arm();
        SILVA_EXPECT_FWD(closure_with_this.define(ti_this, class_instance));
        function_t bound_func{fun.pts};
        bound_func.closure = std::move(closure_with_this);
        return object_pool.make(std::move(bound_func));
      }
    }
    SILVA_EXPECT(false, MINOR, "couldn't access member");
  }

  expected_t<object_ref_t> object_ref_from_literal(const parse_tree_span_t& pts,
                                                   object_pool_t& object_pool,
                                                   const lexicon_t& lexicon)
  {
    using enum token_category_t;
    SILVA_EXPECT(pts[0].rule_name == lexicon.ni_expr_atom, ASSERT);
    const auto ti    = pts.tp->tokens[pts[0].token_begin];
    const auto tinfo = pts.tp->token_info_get(pts[0].token_begin);
    if (ti == lexicon.ti_none) {
      return object_pool.make(none);
    }
    else if (ti == lexicon.ti_true) {
      return object_pool.make(true);
    }
    else if (ti == lexicon.ti_false) {
      return object_pool.make(false);
    }
    else if (tinfo->category == STRING) {
      const auto sov = SILVA_EXPECT_FWD(tinfo->string_as_plain_contained());
      return object_pool.make(string_t{sov});
    }
    else if (tinfo->category == NUMBER) {
      const auto dd = SILVA_EXPECT_FWD(tinfo->number_as_double());
      return object_pool.make(double{dd});
    }
    SILVA_EXPECT(false,
                 MINOR,
                 "{} could not turn literal into lox object {}",
                 pts,
                 lexicon.swp->token_id_wrap(ti));
  }

  // object_t

  bool object_t::is_none() const
  {
    return variant_holds_t<const none_t>{}(data);
  }
  bool object_t::holds_bool() const
  {
    return variant_holds_t<const bool>{}(data);
  }
  bool object_t::holds_double() const
  {
    return variant_holds_t<const double>{}(data);
  }
  bool object_t::holds_string() const
  {
    return variant_holds_t<const string_t>{}(data);
  }
  bool object_t::holds_fundamental() const
  {
    return is_none() || holds_bool() || holds_double() || holds_string();
  }

  bool object_t::holds_function() const
  {
    return variant_holds_t<function_t>{}(data);
  }
  bool object_t::holds_function_builtin() const
  {
    return variant_holds_t<function_builtin_t>{}(data);
  }
  bool object_t::holds_class() const
  {
    return variant_holds_t<class_t>{}(data);
  }
  bool object_t::holds_class_instance() const
  {
    return variant_holds_t<class_instance_t>{}(data);
  }

  expected_t<double> object_t::as_double() const
  {
    SILVA_EXPECT(holds_double(), MINOR, "not a double");
    return std::get<const double>(data);
  }
  expected_t<string_t> object_t::as_string() const
  {
    SILVA_EXPECT(holds_string(), MINOR, "not a string");
    return std::get<const std::string>(data);
  }

  struct clear_scopes_visitor_t {
    void operator()(function_t& x) { x.closure.clear(); }
    void operator()(function_builtin_t& x) { x.closure.clear(); }
    void operator()(class_t& x) {}
    void operator()(class_instance_t& x) {}
    void operator()(auto&) {}
  };
  void object_t::clear_scopes()
  {
    std::visit(clear_scopes_visitor_t{}, data);
  }

  bool object_t::is_truthy() const
  {
    if (is_none()) {
      return false;
    }
    if (holds_bool()) {
      return std::get<const bool>(data);
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
      return -std::get<const double>(x.data);
    }
    else {
      SILVA_EXPECT(false, RUNTIME, "type error evaluating expression: - {} ", pretty_string(x));
    }
  }

#define BINARY_DOUBLE(return_type, op)                                             \
  expected_t<return_type> operator op(const object_t & lhs, const object_t & rhs)  \
  {                                                                                \
    if (lhs.holds_double() && rhs.holds_double()) {                                \
      return std::get<const double>(lhs.data) op std::get<const double>(rhs.data); \
    }                                                                              \
    else {                                                                         \
      SILVA_EXPECT(false,                                                          \
                   RUNTIME,                                                        \
                   "type error evaluating expression: {} " #op " {}",              \
                   pretty_string(lhs),                                             \
                   pretty_string(rhs));                                            \
    }                                                                              \
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
      return std::get<const double>(lhs.data) + std::get<const double>(rhs.data);
    }
    else if (lhs.holds_string() && rhs.holds_string()) {
      return std::get<const string_t>(lhs.data) + std::get<const string_t>(rhs.data);
    }
    else {
      SILVA_EXPECT(false,
                   RUNTIME,
                   "type error evaluating expression: {} + {}",
                   pretty_string(lhs),
                   pretty_string(rhs));
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

  struct object_pretty_write_impl_visitor_t {
    byte_sink_t* byte_sink = nullptr;

    void operator()(const none_t& x) const { byte_sink->write_str("none"); }
    void operator()(const bool& x) const
    {
      if (x) {
        byte_sink->write_str("true");
      }
      else {
        byte_sink->write_str("false");
      }
    }
    void operator()(const double& x) const
    {
      auto retval = std::to_string(x);
      while (retval.size() >= 2 && retval.back() == '0') {
        retval.pop_back();
      }
      if (retval.size() >= 2 && retval.back() == '.') {
        retval.pop_back();
      }
      byte_sink->write_str(retval);
    }
    void operator()(const string_t& x) const { byte_sink->write_str(x); }
    void operator()(const function_t& x) const
    {
      byte_sink->format("<function {}>", pretty_string(x.pts));
    }
    void operator()(const function_builtin_t& x) const
    {
      byte_sink->format("<builtin-function '{}'>", pretty_string(x.pts));
    }
    void operator()(const class_t& x) const
    {
      byte_sink->format("<class {}>", pretty_string(x.pts));
    }
    void operator()(const class_instance_t& x) const
    {
      byte_sink->format("<instance of {}>", pretty_string(x._class));
    }
    void operator()(const auto& x) const { byte_sink->format("Unknown lox::object_t"); }
  };
  void pretty_write_impl(const object_t& value, byte_sink_t* byte_sink)
  {
    return std::visit(object_pretty_write_impl_visitor_t{byte_sink}, value.data);
  }
  std::ostream& operator<<(std::ostream& os, const object_t& x)
  {
    return os << silva::pretty_string(x);
  }

  expected_t<object_ref_t> neg(object_pool_t& object_pool, object_ref_t x)
  {
    return object_pool.make(!(*x));
  }
  expected_t<object_ref_t> inv(object_pool_t& object_pool, object_ref_t x)
  {
    return object_pool.make(SILVA_EXPECT_FWD_PLAIN(-(*x)));
  }
  expected_t<object_ref_t> add(object_pool_t& object_pool, object_ref_t lhs, object_ref_t rhs)
  {
    auto res = SILVA_EXPECT_FWD_PLAIN(*lhs + *rhs);
    return std::visit([&](auto x) { return object_pool.make(std::move(x)); }, std::move(res));
  }
#define OBJECT_IMPL(func_name, op)                                  \
  expected_t<object_ref_t> func_name(object_pool_t& object_pool,    \
                                     object_ref_t lhs,              \
                                     object_ref_t rhs)              \
  {                                                                 \
    return object_pool.make(SILVA_EXPECT_FWD_PLAIN(*lhs op * rhs)); \
  }
  OBJECT_IMPL(sub, -);
  OBJECT_IMPL(mul, *);
  OBJECT_IMPL(div, /);
  OBJECT_IMPL(lt, <);
  OBJECT_IMPL(gt, >);
  OBJECT_IMPL(lte, <=);
  OBJECT_IMPL(gte, >=);
#undef OBJECT_IMPL
  expected_t<object_ref_t> eq(object_pool_t& object_pool, object_ref_t lhs, object_ref_t rhs)
  {
    return object_pool.make(*lhs == *rhs);
  }
  expected_t<object_ref_t> neq(object_pool_t& object_pool, object_ref_t lhs, object_ref_t rhs)
  {
    return object_pool.make(*lhs != *rhs);
  }
}
