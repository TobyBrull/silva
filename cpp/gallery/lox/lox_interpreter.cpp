#include "lox_interpreter.hpp"

using enum silva::token_category_t;

namespace silva::lox {

  struct value_to_string_impl_visitor_t {
    string_or_view_t operator()(const double& x) const
    {
      return string_or_view_t{std::to_string(x)};
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

  expected_t<value_t> operator*(const value_t& lhs, const value_t& rhs)
  {
    SILVA_EXPECT(variant_holds_t<double>{}(lhs.data), MINOR);
    SILVA_EXPECT(variant_holds_t<double>{}(rhs.data), MINOR);
    return {std::get<double>(lhs.data) * std::get<double>(rhs.data)};
  }

  expected_t<value_t> operator+(const value_t& lhs, const value_t& rhs)
  {
    SILVA_EXPECT(variant_holds_t<double>{}(lhs.data), MINOR);
    SILVA_EXPECT(variant_holds_t<double>{}(rhs.data), MINOR);
    return {std::get<double>(lhs.data) + std::get<double>(rhs.data)};
  }

  struct evaluation_t {
    interpreter_t* intp        = nullptr;
    syntax_ward_ptr_t swp      = intp->swp;
    const name_id_style_t& nis = swp->default_name_id_style();

    expected_t<value_t> expr(const parse_tree_span_t pts)
    {
      const name_id_t rn = pts[0].rule_name;
      if (rn == intp->ni_expr_b_mul) {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        const auto lhs_res    = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs)));
        const auto rhs_res    = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs)));
        return lhs_res * rhs_res;
      }
      else if (rn == intp->ni_expr_b_add) {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        const auto lhs_res    = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs)));
        const auto rhs_res    = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs)));
        return lhs_res + rhs_res;
      }
      else {
        SILVA_EXPECT(false,
                     MAJOR,
                     "can't evaluate expression {}",
                     swp->name_id_wrap(pts[0].rule_name));
      }
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
        const value_t value = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(1)));
        fmt::print("{}\n", to_string(value));
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
