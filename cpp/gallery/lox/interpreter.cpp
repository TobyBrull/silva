#include "interpreter.hpp"

using enum silva::token_category_t;

namespace silva::lox {

  struct evaluation_t {
    interpreter_t* intp        = nullptr;
    syntax_ward_ptr_t swp      = intp->swp;
    const name_id_style_t& nis = swp->default_name_id_style();

    expected_t<value_t> expr(const parse_tree_span_t pts)
    {
#define UNARY(op_rule_name, op)                                                            \
  else if (rn == op_rule_name)                                                             \
  {                                                                                        \
    const auto [node_idx] = SILVA_EXPECT_FWD(pts.get_children<1>());                       \
    auto res              = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(node_idx)), \
                                "{} error evaluating unary operand",          \
                                pts);                                         \
    return op std::move(res);                                                              \
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
    return std::move(lhs_res) op std::move(rhs_res);                                  \
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
      return {none};
    }

    expected_t<value_t> atom(const parse_tree_span_t pts)
    {
      const token_id_t ti            = pts.tp->tokens[pts[0].token_begin];
      const token_info_t* token_info = pts.tp->token_info_get(pts[0].token_begin);
      if (ti == intp->ti_true) {
        return {true};
      }
      else if (ti == intp->ti_false) {
        return {false};
      }
      else if (ti == intp->ti_none) {
        return {none};
      }
      else if (token_info->category == STRING) {
        return {string_t{SILVA_EXPECT_FWD(token_info->string_as_plain_contained())}};
      }
      else if (token_info->category == NUMBER) {
        return {double{SILVA_EXPECT_FWD(token_info->number_as_double())}};
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
      return {none};
    }
  };

  expected_t<value_t> interpreter_t::evaluate(const parse_tree_span_t pts)
  {
    evaluation_t eval_run{.intp = this};
    return eval_run.expr_or_atom(pts);
  }

  struct execution_t {
    interpreter_t* intp   = nullptr;
    syntax_ward_ptr_t swp = intp->swp;

    expected_t<void> decl(const parse_tree_span_t pts)
    {
      const name_id_t rule_name = pts[0].rule_name;
      if (false) {
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
        value_t value = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(1)),
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
    execution_t exec_run{.intp = this};
    SILVA_EXPECT_FWD_PLAIN(exec_run.go(pts));
    return {};
  }
}
