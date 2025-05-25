#include "interpreter.hpp"

using enum silva::token_category_t;

namespace silva::lox {
  struct evaluation_t {
    interpreter_t* intp        = nullptr;
    syntax_ward_ptr_t swp      = intp->swp;
    const name_id_style_t& nis = swp->default_name_id_style();
    scope_ptr_t scope;

    expected_t<object_ref_t> expr(const parse_tree_span_t pts)
    {
#define UNARY(op_rule_name, op_func)                                                       \
  else if (rn == op_rule_name)                                                             \
  {                                                                                        \
    const auto [node_idx] = SILVA_EXPECT_FWD(pts.get_children<1>());                       \
    auto res              = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(node_idx)), \
                                "{} error evaluating unary operand",          \
                                pts);                                         \
    return op_func(intp->pool, std::move(res));                                            \
  }

#define BINARY(op_rule_name, op_func)                                                 \
  else if (rn == op_rule_name)                                                        \
  {                                                                                   \
    const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());                  \
    auto lhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs)), \
                                    "{} error evaluating left-hand-side",    \
                                    pts);                                    \
    auto rhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs)), \
                                    "{} error evaluating right-hand-side",   \
                                    pts);                                    \
    return op_func(intp->pool, std::move(lhs_res), std::move(rhs_res));               \
  }

      const name_id_t rn = pts[0].rule_name;
      if (false) {
      }
      UNARY(intp->ni_expr_u_exc, neg)
      UNARY(intp->ni_expr_u_sub, inv)
      BINARY(intp->ni_expr_b_mul, mul)
      BINARY(intp->ni_expr_b_div, div)
      BINARY(intp->ni_expr_b_add, add)
      BINARY(intp->ni_expr_b_sub, sub)
      BINARY(intp->ni_expr_b_lt, lt)
      BINARY(intp->ni_expr_b_gt, gt)
      BINARY(intp->ni_expr_b_lte, lte)
      BINARY(intp->ni_expr_b_gte, gte)
      BINARY(intp->ni_expr_b_eq, eq)
      BINARY(intp->ni_expr_b_neq, neq)
      else if (rn == intp->ni_expr_b_and)
      {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto lhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs)),
                                        "{} error evaluating left-hand-side",
                                        pts);
        if (!lhs_res->is_truthy()) {
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
        if (lhs_res->is_truthy()) {
          return lhs_res;
        }
        auto rhs_res = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs)),
                                        "{} error evaluating right-hand-side",
                                        pts);
        return rhs_res;
      }
      else if (rn == intp->ni_expr_call)
      {
        const auto [fun_idx, args_idx] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto fun_res = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(fun_idx)),
                                        "{} error evaluating left-hand-side",
                                        pts);
        SILVA_EXPECT(fun_res->holds_function(),
                     MINOR,
                     "left-hand-side of call-operator must evaluate to function");
        function_t& fun = std::get<function_t>(fun_res->data);

        const auto pts_args = pts.sub_tree_span_at(args_idx);
        const index_t arity = fun.arity();
        SILVA_EXPECT(arity == pts_args[0].num_children,
                     MINOR,
                     "trying to call <function {}>, which has {} parameters, with {} arguments",
                     fun.pts,
                     arity,
                     pts_args[0].num_children);

        const auto fun_params        = fun.parameters();
        auto [args_it, args_end]     = pts_args.children_range();
        auto [params_it, params_end] = fun_params.children_range();
        auto func_scope              = fun.closure->make_child_scope();
        for (index_t i = 0; i < arity; ++i) {
          SILVA_EXPECT(args_it != args_end && params_it != params_end, MAJOR);
          auto val = SILVA_EXPECT_FWD(expr_or_atom(pts_args.sub_tree_span_at(args_it.pos)),
                                      "{} error evaluating {}-th argument (counted 0-based)",
                                      pts,
                                      i);
          const token_id_t ti_param =
              fun_params.tp->tokens[fun_params.sub_tree_span_at(params_it.pos)[0].token_begin];
          SILVA_EXPECT_FWD(func_scope->define(ti_param, std::move(val)));
          ++args_it;
          ++params_it;
        }
        SILVA_EXPECT(args_it == args_end && params_it == params_end, MAJOR);

        function_t& fun2 = std::get<function_t>(fun_res->data);
        auto res         = SILVA_EXPECT_FWD(intp->execute(fun2.body(), func_scope));
        return res.value_or(intp->pool.make(none));
      }
      else if (rn == intp->ni_expr_b_assign)
      {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        const auto pts_lhs    = pts.sub_tree_span_at(lhs);
        SILVA_EXPECT(pts_lhs[0].rule_name == intp->ni_expr_atom,
                     MAJOR,
                     "{} left-hand-side of assignment must be plain identifier for now",
                     pts_lhs);
        const token_id_t var_name = pts.tp->tokens[pts_lhs[0].token_begin];
        auto new_val              = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs)),
                                        "{} error evaluating right-hand-side of assignment",
                                        pts);
        SILVA_EXPECT_FWD(scope->assign(var_name, std::move(new_val)));
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
      return intp->pool.make(none);
    }

    expected_t<object_ref_t> atom(const parse_tree_span_t pts)
    {
      const token_id_t ti            = pts.tp->tokens[pts[0].token_begin];
      const token_info_t* token_info = pts.tp->token_info_get(pts[0].token_begin);
      if (ti == intp->ti_true) {
        return intp->pool.make(true);
      }
      else if (ti == intp->ti_false) {
        return intp->pool.make(false);
      }
      else if (ti == intp->ti_none) {
        return intp->pool.make(none);
      }
      else if (token_info->category == STRING) {
        return intp->pool.make(string_t{SILVA_EXPECT_FWD(token_info->string_as_plain_contained())});
      }
      else if (token_info->category == NUMBER) {
        return intp->pool.make(double{SILVA_EXPECT_FWD(token_info->number_as_double())});
      }
      else if (token_info->category == IDENTIFIER) {
        auto ref = SILVA_EXPECT_FWD(scope->get(ti));
        return ref;
      }
      else {
        SILVA_EXPECT(false,
                     MAJOR,
                     "token {} is not a {}",
                     swp->token_id_wrap(ti),
                     swp->name_id_wrap(pts[0].rule_name));
      }
    }

    expected_t<object_ref_t> expr_or_atom(const parse_tree_span_t pts)
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
      return intp->pool.make(none);
    }
  };

  expected_t<object_ref_t> interpreter_t::evaluate(const parse_tree_span_t pts, scope_ptr_t scope)
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

    scope_ptr_t scope;

    expected_t<void> decl(const parse_tree_span_t pts)
    {
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_decl_var) {
        const token_id_t var_name = pts.tp->tokens[pts[0].token_begin + 1];
        const auto children       = SILVA_EXPECT_FWD(pts.get_children_up_to<1>());
        object_ref_t initializer;
        if (children.size == 1) {
          initializer = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(children[0]), scope));
        }
        SILVA_EXPECT_FWD(scope->define(var_name, std::move(initializer)));
      }
      else if (rule_name == intp->ni_decl_fun) {
        const token_id_t fun_name = pts.tp->tokens[pts[0].token_begin + 1];
        SILVA_EXPECT(pts[0].num_children == 1, MAJOR);
        const auto func_pts = pts.sub_tree_span_at(1);
        SILVA_EXPECT_FWD(scope->define(fun_name, intp->pool.make(function_t{func_pts, scope})));
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown declaration {}", pts, swp->name_id_wrap(rule_name));
      }
      return {};
    }

    expected_t<return_t<object_ref_t>> stmt(const parse_tree_span_t pts)
    {
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_stmt_print) {
        object_ref_t value = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(1), scope),
                                              "{} error evaluating argument to 'print'",
                                              pts);
        fmt::println("{}", to_string(std::move(value)));
      }
      else if (rule_name == intp->ni_stmt_if) {
        auto [it, end] = pts.children_range();
        SILVA_EXPECT(it != end, MAJOR);
        pts.sub_tree_span_at(it.pos);
        const auto cond = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(it.pos), scope),
                                           "{} error evaluating if-condition",
                                           pts);
        ++it;
        SILVA_EXPECT(it != end, MAJOR);
        if (cond->is_truthy()) {
          auto res = SILVA_EXPECT_FWD(intp->execute(pts.sub_tree_span_at(it.pos), scope));
          if (res.has_value()) {
            return res;
          }
        }
        else {
          ++it;
          if (it != end) {
            auto res = SILVA_EXPECT_FWD(intp->execute(pts.sub_tree_span_at(it.pos), scope));
            if (res.has_value()) {
              return res;
            }
          }
        }
      }
      else if (rule_name == intp->ni_stmt_for) {
        const auto [init_idx, cond_idx, inc_idx, body_idx] =
            SILVA_EXPECT_FWD(pts.get_children<4>());
        // TODO: make nested scope?
        SILVA_EXPECT_FWD(intp->execute(pts.sub_tree_span_at(init_idx), scope),
                         "{} error evaluating for-initializer",
                         pts);
        while (true) {
          const auto cond = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(cond_idx), scope),
                                             "{} error evaluating for-condition",
                                             pts);
          if (!cond->is_truthy()) {
            break;
          }
          auto res = SILVA_EXPECT_FWD(intp->execute(pts.sub_tree_span_at(body_idx), scope));
          if (res.has_value()) {
            return res;
          }
          SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(inc_idx), scope),
                           "{} error evaluating for-condition",
                           pts);
        }
      }
      else if (rule_name == intp->ni_stmt_return) {
        auto res = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(1), scope),
                                    "{} error evaluating expression of return statement",
                                    pts);
        return {return_t<object_ref_t>{std::move(res)}};
      }
      else if (rule_name == intp->ni_stmt_block) {
        // TODO: make block scope
        for (const auto [node_idx, child_idx]: pts.children_range()) {
          auto res = SILVA_EXPECT_FWD_PLAIN(go(pts.sub_tree_span_at(node_idx)));
          if (res.has_value()) {
            return res;
          }
        }
      }
      else if (rule_name == intp->ni_stmt_expr) {
        SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(1), scope),
                         "{} error evaluating expression statement",
                         pts);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown statement {}", pts, swp->name_id_wrap(rule_name));
      }
      return {{std::nullopt}};
    }

    expected_t<return_t<object_ref_t>> go(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(pts.size() > 0, MAJOR);
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_none) {
        ;
      }
      else if (rule_name == intp->ni_lox) {
        for (const auto [node_idx, child_idx]: pts.children_range()) {
          SILVA_EXPECT_FWD_PLAIN(go(pts.sub_tree_span_at(node_idx)));
        }
      }
      else if (rule_name == intp->ni_decl) {
        SILVA_EXPECT_FWD(decl(pts.sub_tree_span_at(1)));
      }
      else if (swp->name_infos[rule_name].parent_name == intp->ni_decl) {
        SILVA_EXPECT_FWD(decl(pts));
      }
      else if (rule_name == intp->ni_stmt) {
        return stmt(pts.sub_tree_span_at(1));
      }
      else if (swp->name_infos[rule_name].parent_name == intp->ni_stmt) {
        return stmt(pts);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown rule {}", pts, swp->name_id_wrap(rule_name));
      }
      return {};
    }
  };

  expected_t<return_t<object_ref_t>> interpreter_t::execute(const parse_tree_span_t pts,
                                                            scope_ptr_t scope)
  {
    execution_t exec_run{.intp = this, .scope = scope};
    return exec_run.go(pts);
  }
}
