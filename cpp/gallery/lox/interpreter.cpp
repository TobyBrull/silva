#include "interpreter.hpp"

using enum silva::token_category_t;

namespace silva::lox {

  expected_t<const value_t*> scope_t::get(const token_id_t ti) const
  {
    const scope_t* sp = this;
    while (true) {
      const auto it = sp->values.find(ti);
      if (it == sp->values.end()) {
        SILVA_EXPECT(parent, MINOR, "couldn't find identifier {}", swp->token_id_wrap(ti));
        sp = parent.get();
      }
      else {
        return {&it->second};
      }
    }
  }
  expected_t<void> scope_t::assign(const token_id_t ti, value_t x)
  {
    scope_t* sp = this;
    while (true) {
      auto it = sp->values.find(ti);
      if (it == sp->values.end()) {
        SILVA_EXPECT(parent,
                     MINOR,
                     "couldn't find identifier {} (trying to assign {} to it)",
                     swp->token_id_wrap(ti),
                     to_string(x));
        sp = parent.get();
      }
      else {
        it->second = std::move(x);
      }
    }
  }
  expected_t<void> scope_t::define(const token_id_t ti, value_t x)
  {
    const auto [it, inserted] = values.emplace(ti, value_t{});
    SILVA_EXPECT(inserted,
                 MINOR,
                 "couldn't define identifier {} (with initializer {}) because the value already "
                 "exists in the local scope",
                 swp->token_id_wrap(ti),
                 to_string(x));
    it->second = std::move(x);
    return {};
  }

  struct evaluation_t {
    interpreter_t* intp        = nullptr;
    syntax_ward_ptr_t swp      = intp->swp;
    const name_id_style_t& nis = swp->default_name_id_style();
    scope_ptr_t scope;

    expected_t<value_t> expr(const parse_tree_span_t pts)
    {
#define UNARY(op_rule_name, op)                                                                  \
  else if (rn == op_rule_name)                                                                   \
  {                                                                                              \
    const auto [node_idx] = SILVA_EXPECT_FWD(pts.get_children<1>());                             \
    auto res              = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(node_idx)),       \
                                "{} error evaluating unary operand",                \
                                pts);                                               \
    return expectify(op std::move(res)).transform([](auto x) { return value_t{std::move(x)}; }); \
  }

#define BINARY(op_rule_name, op)                                                      \
  else if (rn == op_rule_name)                                                        \
  {                                                                                   \
    const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());                  \
    auto lhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs)), \
                                    "{} error evaluating left-hand-side",    \
                                    pts);                                    \
    auto rhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs)), \
                                    "{} error evaluating right-hand-side",   \
                                    pts);                                    \
    return expectify(std::move(lhs_res) op std::move(rhs_res)).transform([](auto x) { \
      return value_t{std::move(x)};                                                   \
    });                                                                               \
  }

      const name_id_t rn = pts[0].rule_name;
      if (false) {
      }
      UNARY(intp->ni_expr_u_exc, !)
      UNARY(intp->ni_expr_u_sub, -)
      BINARY(intp->ni_expr_b_mul, *)
      BINARY(intp->ni_expr_b_div, /)
      BINARY(intp->ni_expr_b_add, +)
      BINARY(intp->ni_expr_b_sub, -)
      BINARY(intp->ni_expr_b_lt, <)
      BINARY(intp->ni_expr_b_gt, >)
      BINARY(intp->ni_expr_b_lte, <=)
      BINARY(intp->ni_expr_b_gte, >=)
      BINARY(intp->ni_expr_b_eq, ==)
      BINARY(intp->ni_expr_b_neq, !=)
      else if (rn == intp->ni_expr_b_and)
      {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto lhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs)),
                                        "{} error evaluating left-hand-side",
                                        pts);
        if (!lhs_res.is_truthy()) {
          return lhs_res;
        }
        auto rhs_res = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs)),
                                        "{} error evaluating right-hand-side",
                                        pts);
        return rhs_res;
      }
      else if (rn == intp->ni_expr_b_or)
      {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto lhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs)),
                                        "{} error evaluating left-hand-side",
                                        pts);
        if (lhs_res.is_truthy()) {
          return lhs_res;
        }
        auto rhs_res = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs)),
                                        "{} error evaluating right-hand-side",
                                        pts);
        return rhs_res;
      }
      else
      {
        SILVA_EXPECT(false,
                     MAJOR,
                     "can't evaluate expression {}",
                     swp->name_id_wrap(pts[0].rule_name));
      }

#undef BINARY
#undef UNARY
      return value_t{none};
    }

    expected_t<value_t> atom(const parse_tree_span_t pts)
    {
      const token_id_t ti            = pts.tp->tokens[pts[0].token_begin];
      const token_info_t* token_info = pts.tp->token_info_get(pts[0].token_begin);
      if (ti == intp->ti_true) {
        return value_t{true};
      }
      else if (ti == intp->ti_false) {
        return value_t{false};
      }
      else if (ti == intp->ti_none) {
        return value_t{none};
      }
      else if (token_info->category == STRING) {
        return value_t{string_t{SILVA_EXPECT_FWD(token_info->string_as_plain_contained())}};
      }
      else if (token_info->category == NUMBER) {
        return value_t{double{SILVA_EXPECT_FWD(token_info->number_as_double())}};
      }
      else if (token_info->category == IDENTIFIER) {
        const value_t* val = SILVA_EXPECT_FWD(scope->get(ti));
        return *val;
      }
      else {
        SILVA_EXPECT(false,
                     MAJOR,
                     "token {} is not a {}",
                     swp->token_id_wrap(ti),
                     swp->name_id_wrap(pts[0].rule_name));
      }
    }

    expected_t<value_t> expr_or_atom(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(pts.size() > 0, MAJOR);
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_expr_atom) {
        return atom(pts);
      }
      else if (swp->name_id_is_parent(intp->ni_expr, rule_name)) {
        return expr(pts);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "can't evaluate {}", swp->name_id_wrap(rule_name));
      }
      return value_t{none};
    }
  };

  expected_t<value_t> interpreter_t::evaluate(const parse_tree_span_t pts, scope_ptr_t scope)
  {
    evaluation_t eval_run{
        .intp  = this,
        .scope = scope,
    };
    return eval_run.expr_or_atom(pts);
  }

  struct execution_t {
    interpreter_t* intp   = nullptr;
    syntax_ward_ptr_t swp = intp->swp;

    scope_ptr_t current_scope;

    expected_t<void> decl(const parse_tree_span_t pts)
    {
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_decl_var) {
        const auto children       = SILVA_EXPECT_FWD(pts.get_children_up_to<1>());
        const token_id_t var_name = pts.tp->tokens[pts[0].token_begin + 1];
        value_t initializer;
        if (children.size == 1) {
          initializer =
              SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(children[0]), current_scope));
        }
        SILVA_EXPECT_FWD(current_scope->define(var_name, std::move(initializer)));
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} can't execute {}", pts, swp->name_id_wrap(rule_name));
      }
      return {};
    }

    expected_t<void> stmt(const parse_tree_span_t pts)
    {
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_stmt_print) {
        value_t value = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(1), current_scope),
                                         "{} error evaluating argument to 'print'",
                                         pts);
        fmt::print("{}\n", to_string(std::move(value)));
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} can't execute {}", pts, swp->name_id_wrap(rule_name));
      }
      return {};
    }

    expected_t<void> go(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(pts.size() > 0, MAJOR);
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_lox) {
        for (const auto [node_idx, child_idx]: pts.children_range()) {
          SILVA_EXPECT_FWD_PLAIN(go(pts.sub_tree_span_at(node_idx)));
        }
      }
      else if (rule_name == intp->ni_decl) {
        return decl(pts.sub_tree_span_at(1));
      }
      else if (rule_name == intp->ni_stmt) {
        return stmt(pts.sub_tree_span_at(1));
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} can't execute {}", pts, swp->name_id_wrap(rule_name));
      }
      return {};
    }
  };

  expected_t<void> interpreter_t::execute(const parse_tree_span_t pts)
  {
    execution_t exec_run{.intp = this, .current_scope = globals};
    SILVA_EXPECT_FWD_PLAIN(exec_run.go(pts));
    return {};
  }
}
