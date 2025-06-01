#include "interpreter.hpp"
#include "syntax/tokenization.hpp"

#include <chrono>
#include <cstdio>

using enum silva::token_category_t;

namespace silva::lox {
  const string_view_t builtins_lox_str = R"'(
    fun clock() {}
    fun getc() {}
    fun chr(ascii_code) {}
    fun exit(exit_code) {}
    fun print_error(text) {}
  )'";

  expected_t<void> interpreter_t::load_builtins(const parser_t& parser)
  {
    const tokenization_ptr_t builtin_tt =
        SILVA_EXPECT_FWD(tokenize(swp, "builtins.lox", builtins_lox_str));
    const auto pts_builtin = SILVA_EXPECT_FWD(parser(builtin_tt, swp->name_id_of("Lox")))->span();

    struct builtin_decl_t {
      token_id_t name = token_id_none;
      silva::function_t<object_ref_t(object_pool_t&, scope_ptr_t)> impl;
    };
    vector_t<builtin_decl_t> builtin_decls{
        builtin_decl_t{
            .name = swp->token_id("clock").value(),
            .impl = [](object_pool_t& pool, scope_ptr_t) -> object_ref_t {
              const auto now      = std::chrono::system_clock::now();
              const auto duration = now.time_since_epoch();
              const double retval =
                  std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
              return pool.make(retval);
            },
        },
        builtin_decl_t{
            .name = swp->token_id("getc").value(),
            .impl = [](object_pool_t& pool, scope_ptr_t) -> object_ref_t {
              const char retval_char = getchar();
              return pool.make(double(retval_char));
            },
        },
        builtin_decl_t{
            .name = swp->token_id("chr").value(),
            .impl = [swp = swp, ti_ascii_code = swp->token_id("ascii_code").value()](
                        object_pool_t& pool,
                        scope_ptr_t scope) -> object_ref_t {
              const object_ref_t val = *SILVA_EXPECT_ASSERT(scope.get(ti_ascii_code));
              SILVA_ASSERT(val->holds_double());
              const double double_val = std::get<double>(val->data);
              string_t retval{(char)double_val};
              return pool.make(retval);
            },
        },
        builtin_decl_t{
            .name = swp->token_id("exit").value(),
            .impl = [swp = swp](object_pool_t& pool, scope_ptr_t scope) -> object_ref_t {
              const object_ref_t val =
                  *SILVA_EXPECT_ASSERT(scope.get(swp->token_id("exit_code").value()));
              SILVA_ASSERT(val->holds_double());
              const double double_val = std::get<double>(val->data);
              std::exit((int)double_val);
            },
        },
        builtin_decl_t{
            .name = swp->token_id("print_error").value(),
            .impl = [swp = swp](object_pool_t& pool, scope_ptr_t scope) -> object_ref_t {
              const object_ref_t val =
                  *SILVA_EXPECT_ASSERT(scope.get(swp->token_id("text").value()));
              SILVA_ASSERT(val->holds_string());
              const auto& text = std::get<string_t>(val->data);
              fmt::println("ERROR: {}", text);
              return pool.make(none);
            },
        },
    };

    auto [it, end] = pts_builtin.children_range();
    for (const builtin_decl_t& builtin_decl: builtin_decls) {
      SILVA_EXPECT(it != end, ASSERT);
      const auto pts_function =
          pts_builtin.sub_tree_span_at(it.pos).sub_tree_span_at(1).sub_tree_span_at(1);
      const token_id_t lox_name = pts_function.tp->tokens[pts_function[0].token_begin];
      SILVA_EXPECT(lox_name == builtin_decl.name,
                   ASSERT,
                   "expected function '{}', but found '{}'",
                   swp->token_id_wrap(lox_name),
                   swp->token_id_wrap(builtin_decl.name));
      object_ref_t obj_ref = pool.make(function_builtin_t{
          {
              .pts     = pts_function,
              .closure = scopes.root(),
          },
          builtin_decl.impl,
      });
      ++it;
      globals = SILVA_EXPECT_ASSERT(globals.define(lox_name, std::move(obj_ref)));
    }

    return {};
  }

  struct evaluation_t {
    interpreter_t* intp        = nullptr;
    syntax_ward_ptr_t swp      = intp->swp;
    const name_id_style_t& nis = swp->default_name_id_style();
    scope_ptr_t scope;

    template<typename Func>
    expected_t<object_ref_t>
    call_function(Func& fun, const parse_tree_span_t pts_args, const bool access_creates)
    {
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
      auto func_scope              = fun.closure.new_scope();
      for (index_t i = 0; i < arity; ++i) {
        SILVA_EXPECT(args_it != args_end && params_it != params_end, MAJOR);
        auto val =
            SILVA_EXPECT_FWD(expr_or_atom(pts_args.sub_tree_span_at(args_it.pos), access_creates),
                             "{} error evaluating {}-th argument (counted 0-based)",
                             pts_args,
                             i);
        const token_id_t ti_param =
            fun_params.tp->tokens[fun_params.sub_tree_span_at(params_it.pos)[0].token_begin];
        func_scope = SILVA_EXPECT_FWD(func_scope.define(ti_param, std::move(val)));
        ++args_it;
        ++params_it;
      }
      SILVA_EXPECT(args_it == args_end && params_it == params_end, MAJOR);
      if constexpr (std::same_as<Func, function_t>) {
        auto result = SILVA_EXPECT_FWD(intp->execute(fun.body(), func_scope));
        return result.value_or(intp->pool.make(none));
      }
      else if constexpr (std::same_as<Func, function_builtin_t>) {
        return fun.impl(intp->pool, func_scope);
      }
      else {
        static_assert(false, "unexpected");
      }
    }

    expected_t<object_ref_t> expr(const parse_tree_span_t pts, const bool ac)
    {
#define UNARY(op_rule_name, op_func)                                                           \
  else if (rn == op_rule_name)                                                                 \
  {                                                                                            \
    const auto [node_idx] = SILVA_EXPECT_FWD(pts.get_children<1>());                           \
    auto res              = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(node_idx), ac), \
                                "{} error evaluating unary operand",              \
                                pts);                                             \
    return op_func(intp->pool, std::move(res));                                                \
  }

#define BINARY(op_rule_name, op_func)                                                     \
  else if (rn == op_rule_name)                                                            \
  {                                                                                       \
    const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());                      \
    auto lhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs), ac), \
                                    "{} error evaluating left-hand-side",        \
                                    pts);                                        \
    auto rhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs), ac), \
                                    "{} error evaluating right-hand-side",       \
                                    pts);                                        \
    return op_func(intp->pool, std::move(lhs_res), std::move(rhs_res));                   \
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
        auto lhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs), ac),
                                        "{} error evaluating left-hand-side",
                                        pts);
        if (!lhs_res->is_truthy()) {
          return lhs_res;
        }
        auto rhs_res = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs), ac),
                                        "{} error evaluating right-hand-side",
                                        pts);
        return rhs_res;
      }
      else if (rn == intp->ni_expr_b_or)
      {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto lhs_res          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs), ac),
                                        "{} error evaluating left-hand-side",
                                        pts);
        if (lhs_res->is_truthy()) {
          return lhs_res;
        }
        auto rhs_res = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs), ac),
                                        "{} error evaluating right-hand-side",
                                        pts);
        return rhs_res;
      }
      else if (rn == intp->ni_expr_call)
      {
        const auto [fun_idx, args_idx] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto callee = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(fun_idx), ac),
                                       "{} error evaluating left-hand-side",
                                       pts);
        SILVA_EXPECT(callee->holds_function() || callee->holds_function_builtin() ||
                         callee->holds_class(),
                     MINOR,
                     "left-hand-side of call-operator must evaluate to function or class");
        const auto pts_args = pts.sub_tree_span_at(args_idx);
        if (callee->holds_function()) {
          function_t& fun = std::get<function_t>(callee->data);
          return call_function(fun, pts_args, ac);
        }
        else if (callee->holds_function_builtin()) {
          function_builtin_t& fun = std::get<function_builtin_t>(callee->data);
          return call_function(fun, pts_args, ac);
        }
        else if (callee->holds_class()) {
          object_ref_t retval = intp->pool.make(class_instance_t{._class = callee});
          class_t& cc         = std::get<class_t>(callee->data);
          if (const auto it = cc.methods.find(intp->ti_init); it != cc.methods.end()) {
            object_ref_t init_fun_ref = SILVA_EXPECT_FWD(
                member_access(retval, intp->ti_init, false, intp->pool, intp->ti_this));
            SILVA_EXPECT(init_fun_ref->holds_function(), MINOR);
            function_t& init_fun = std::get<function_t>(init_fun_ref->data);
            SILVA_EXPECT_FWD(call_function(init_fun, pts_args, ac));
          }
          return retval;
        }
      }
      else if (rn == intp->ni_expr_member)
      {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto lhs_ref =
            SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs), ac),
                             "{} error evaluating left-hand-side of member-access operator",
                             pts);
        const auto pts_rhs = pts.sub_tree_span_at(rhs);
        SILVA_EXPECT(pts_rhs[0].rule_name == intp->ni_expr_atom,
                     MAJOR,
                     "{} right-hand-side of member-access operator must be plain identifier",
                     pts_rhs);
        const token_id_t field_name = pts.tp->tokens[pts_rhs[0].token_begin];
        return SILVA_EXPECT_FWD(member_access(lhs_ref, field_name, ac, intp->pool, intp->ti_this),
                                "{} could not find {} in {}",
                                pts,
                                swp->token_id_wrap(field_name),
                                to_string(lhs_ref));
      }
      else if (rn == intp->ni_expr_b_assign)
      {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto lhs_ref          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs), true),
                                        "{} error evaluating left-hand-side of assignment",
                                        pts);
        auto rhs_ref          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs), ac),
                                        "{} error evaluating right-hand-side of assignment",
                                        pts);
        *lhs_ref              = *rhs_ref;
      }
      else if (rn == intp->ni_expr_primary)
      {
        return expr_or_atom(pts.sub_tree_span_at(1), ac);
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
        auto ref = SILVA_EXPECT_FWD(scope.get(ti),
                                    "trying to resolve variable {}",
                                    swp->token_id_wrap(ti));
        return *ref;
      }
      else {
        SILVA_EXPECT(false,
                     MAJOR,
                     "token {} is not a {}",
                     swp->token_id_wrap(ti),
                     swp->name_id_wrap(pts[0].rule_name));
      }
    }

    expected_t<object_ref_t> expr_or_atom(const parse_tree_span_t pts, const bool access_creates)
    {
      SILVA_EXPECT(pts.size() > 0, MAJOR);
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_expr_atom) {
        return atom(pts);
      }
      else if (swp->name_id_is_parent(intp->ni_expr, rule_name)) {
        return expr(pts, access_creates);
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
    return eval_run.expr_or_atom(pts, false);
  }

  struct execution_t {
    interpreter_t* intp   = nullptr;
    syntax_ward_ptr_t swp = intp->swp;

    expected_t<return_t<object_ref_t>> decl(const parse_tree_span_t pts, scope_ptr_t& scope)
    {
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_decl_var) {
        const token_id_t var_name = pts.tp->tokens[pts[0].token_begin + 1];
        const auto children       = SILVA_EXPECT_FWD(pts.get_children_up_to<1>());
        object_ref_t initializer  = intp->pool.make(none);
        if (children.size == 1) {
          initializer = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(children[0]), scope),
                                         "{} when evaluating initializer of variable declaration",
                                         pts);
        }
        scope = SILVA_EXPECT_FWD(scope.define(var_name, std::move(initializer)),
                                 "{} error defining variable {}",
                                 pts,
                                 swp->token_id_wrap(var_name));
      }
      else if (rule_name == intp->ni_decl_fun) {
        const token_id_t fun_name = pts.tp->tokens[pts[0].token_begin + 1];
        scope                     = SILVA_EXPECT_FWD(scope.define(fun_name, intp->pool.make(none)));
        SILVA_EXPECT(pts[0].num_children == 1, MAJOR);
        const auto func_pts = pts.sub_tree_span_at(1);
        SILVA_EXPECT_FWD(scope.assign(fun_name, intp->pool.make(function_t{func_pts, scope})));
      }
      else if (rule_name == intp->ni_decl_class) {
        const token_id_t class_name = pts.tp->tokens[pts[0].token_begin + 1];
        scope          = SILVA_EXPECT_FWD(scope.define(class_name, intp->pool.make(none)));
        auto [it, end] = pts.children_range();
        SILVA_EXPECT(it != end, MAJOR);
        const auto pts_super = pts.sub_tree_span_at(it.pos);
        SILVA_EXPECT(pts_super[0].rule_name == intp->ni_decl_class_s, MAJOR);
        class_t cc;
        cc.name = class_name;
        cc.pts  = pts;
        ++it;
        while (it != end) {
          const auto pts_method = pts.sub_tree_span_at(it.pos);
          SILVA_EXPECT(pts_method[0].rule_name == intp->ni_decl_function, MAJOR);
          const token_id_t method_name = pts.tp->tokens[pts_method[0].token_begin];
          cc.methods[method_name]      = intp->pool.make(function_t{pts_method, scope});
          ++it;
        }
        SILVA_EXPECT_FWD(scope.assign(class_name, intp->pool.make(std::move(cc))));
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown declaration {}", pts, swp->name_id_wrap(rule_name));
      }
      return {std::nullopt};
    }

    expected_t<return_t<object_ref_t>> stmt(const parse_tree_span_t pts, scope_ptr_t& scope)
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
        auto res = SILVA_EXPECT_FWD(intp->execute(pts.sub_tree_span_at(init_idx), scope),
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
      else if (rule_name == intp->ni_stmt_while) {
        const auto [cond_idx, body_idx] = SILVA_EXPECT_FWD(pts.get_children<2>());
        // TODO: make nested scope?
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
        }
      }
      else if (rule_name == intp->ni_stmt_return) {
        auto res = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(1), scope),
                                    "{} error evaluating expression of return statement",
                                    pts);
        return return_t<object_ref_t>{std::move(res)};
      }
      else if (rule_name == intp->ni_stmt_block) {
        scope = scope.new_scope();
        for (const auto [node_idx, child_idx]: pts.children_range()) {
          auto res = SILVA_EXPECT_FWD_PLAIN(go(pts.sub_tree_span_at(node_idx), scope));
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
      return {std::nullopt};
    }

    expected_t<return_t<object_ref_t>> go(const parse_tree_span_t pts, scope_ptr_t& scope)
    {
      SILVA_EXPECT(pts.size() > 0, MAJOR);
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->ni_none) {
        ;
      }
      else if (rule_name == intp->ni_lox) {
        for (const auto [node_idx, child_idx]: pts.children_range()) {
          auto res = SILVA_EXPECT_FWD_PLAIN(go(pts.sub_tree_span_at(node_idx), scope));
        }
      }
      else if (rule_name == intp->ni_decl) {
        return decl(pts.sub_tree_span_at(1), scope);
      }
      else if (swp->name_infos[rule_name].parent_name == intp->ni_decl) {
        return decl(pts, scope);
      }
      else if (rule_name == intp->ni_stmt) {
        return stmt(pts.sub_tree_span_at(1), scope);
      }
      else if (swp->name_infos[rule_name].parent_name == intp->ni_stmt) {
        return stmt(pts, scope);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown rule {}", pts, swp->name_id_wrap(rule_name));
      }
      return {std::nullopt};
    }
  };

  expected_t<return_t<object_ref_t>> interpreter_t::execute(const parse_tree_span_t pts,
                                                            scope_ptr_t& scope)
  {
    execution_t exec_run{.intp = this};
    return exec_run.go(pts, scope);
  }
}
