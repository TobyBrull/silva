#include "interpreter.hpp"
#include "canopy/scope_exit.hpp"
#include "syntax/tokenization.hpp"

#include <chrono>
#include <cstdio>

using enum silva::token_category_t;

namespace silva::lox {

  interpreter_t::interpreter_t(syntax_ward_ptr_t swp, byte_sink_t* byte_sink)
    : lexicon(std::move(swp)), print_stream(byte_sink)
  {
  }

  interpreter_t::~interpreter_t()
  {
    object_pool.for_each_object([&](object_t& obj) { obj.clear_scopes(); });
  }

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
        SILVA_EXPECT_FWD(tokenize(lexicon.swp, "builtins.lox", builtins_lox_str));
    const auto pts_builtin =
        SILVA_EXPECT_FWD(parser(builtin_tt, lexicon.swp->name_id_of("Lox")))->span();

    struct builtin_decl_t {
      token_id_t name = token_id_none;
      silva::function_t<object_ref_t(object_pool_t&, scope_ptr_t)> impl;
    };
    vector_t<builtin_decl_t> builtin_decls{
        builtin_decl_t{
            .name = lexicon.swp->token_id("clock").value(),
            .impl = [](object_pool_t& object_pool, scope_ptr_t) -> object_ref_t {
              const auto now      = std::chrono::system_clock::now();
              const auto duration = now.time_since_epoch();
              const double retval =
                  std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
              return object_pool.make(retval);
            },
        },
        builtin_decl_t{
            .name = lexicon.swp->token_id("getc").value(),
            .impl = [](object_pool_t& object_pool, scope_ptr_t) -> object_ref_t {
              const char retval_char = getchar();
              return object_pool.make(double(retval_char));
            },
        },
        builtin_decl_t{
            .name = lexicon.swp->token_id("chr").value(),
            .impl = [swp           = lexicon.swp,
                     ti_ascii_code = lexicon.swp->token_id("ascii_code").value()](
                        object_pool_t& object_pool,
                        scope_ptr_t scope) -> object_ref_t {
              const object_ref_t* ptr = scope.get(ti_ascii_code);
              SILVA_ASSERT(ptr != nullptr, "couldn't find parameter 'ascii_code'");
              const object_ref_t& val = *ptr;
              SILVA_ASSERT(val->holds_double());
              const double double_val = std::get<const double>(val->data);
              string_t retval{(char)double_val};
              return object_pool.make(retval);
            },
        },
        builtin_decl_t{
            .name = lexicon.swp->token_id("exit").value(),
            .impl = [swp = lexicon.swp, ti_exit_code = lexicon.swp->token_id("exit_code").value()](
                        object_pool_t& object_pool,
                        scope_ptr_t scope) -> object_ref_t {
              const object_ref_t* ptr = scope.get(ti_exit_code);
              SILVA_ASSERT(ptr != nullptr, "couldn't find parameter 'exit_code'");
              const object_ref_t val = *ptr;
              SILVA_ASSERT(val->holds_double());
              const double double_val = std::get<const double>(val->data);
              std::exit((int)double_val);
            },
        },
        builtin_decl_t{
            .name = lexicon.swp->token_id("print_error").value(),
            .impl = [swp = lexicon.swp, ti_text = lexicon.swp->token_id("text").value()](
                        object_pool_t& object_pool,
                        scope_ptr_t scope) -> object_ref_t {
              const object_ref_t* ptr = scope.get(ti_text);
              SILVA_ASSERT(ptr != nullptr, "couldn't find parameter 'text'");
              const object_ref_t val = *ptr;
              SILVA_ASSERT(val->holds_string());
              const auto& text = std::get<const string_t>(val->data);
              fmt::println("ERROR: {}", text);
              return object_pool.make(none);
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
                   lexicon.swp->token_id_wrap(lox_name),
                   lexicon.swp->token_id_wrap(builtin_decl.name));
      function_builtin_t fb{{pts_function}, builtin_decl.impl};
      fb.closure           = scopes.root();
      object_ref_t obj_ref = object_pool.make(std::move(fb));
      ++it;
      SILVA_EXPECT_ASSERT(scopes.root().define(lox_name, std::move(obj_ref)));
    }

    return {};
  }

  expected_t<void> interpreter_t::resolve(parse_tree_span_t pts)
  {
    enum class_type_t {
      NONE,
      CLASS,
      SUBCLASS,
    };
    struct static_scope_t {
      hash_map_t<token_id_t, bool> variables;
      class_type_t class_type = NONE;
    };
    vector_t<static_scope_t> static_scopes;
    static_scopes.push_back({});

    const auto count_variable = [&](const parse_tree_span_t ti_pts,
                                    const token_id_t ti) -> optional_t<index_t> {
      index_t n      = static_scopes.size();
      index_t retval = 0;
      for (; retval < n; ++retval) {
        if (static_scopes[n - 1 - retval].variables.contains(ti)) {
          return retval;
        }
      }
      return {none};
    };

    auto result = pts.visit_subtree([&](const span_t<const tree_branch_t> path,
                                        const tree_event_t event) -> expected_t<bool> {
      SILVA_EXPECT(static_scopes.size() >= 1, ASSERT);
      const auto pts_curr = pts.sub_tree_span_at(path.back().node_index);
      if (pts_curr[0].rule_name == lexicon.ni_stmt_block) {
        if (is_on_entry(event)) {
          static_scopes.push_back(static_scope_t{.class_type = static_scopes.back().class_type});
        }
        if (is_on_exit(event)) {
          static_scopes.pop_back();
        }
      }
      else if (pts_curr[0].rule_name == lexicon.ni_decl_var) {
        const auto ti = pts.tp->tokens[pts_curr[0].token_begin + 1];
        if (is_on_entry(event)) {
          static_scopes.back().variables.emplace(ti, false);
        }
        if (is_on_exit(event)) {
          static_scopes.back().variables[ti] = true;
        }
      }
      else if (pts_curr[0].rule_name == lexicon.ni_decl_class) {
        const auto pts_super = pts_curr.sub_tree_span_at(1);
        const bool has_super = (pts_super[0].token_begin < pts_super[0].token_end);
        if (is_on_entry(event)) {
          const auto ti = pts.tp->tokens[pts_curr[0].token_begin + 1];
          static_scopes.back().variables.emplace(ti, true);
          static_scopes.push_back(static_scope_t{.class_type = CLASS});
          if (has_super) {
            static_scopes.back().variables.emplace(lexicon.ti_super, true);
            static_scopes.push_back(static_scope_t{.class_type = SUBCLASS});
          }
          static_scopes.back().variables.emplace(lexicon.ti_this, true);
        }
        if (is_on_exit(event)) {
          static_scopes.pop_back();
          if (has_super) {
            static_scopes.pop_back();
          }
        }
      }
      else if (pts_curr[0].rule_name == lexicon.ni_decl_class_s) {
        SILVA_EXPECT(path.size() >= 2, MAJOR);
        if (pts_curr[0].token_begin < pts_curr[0].token_end) {
          const auto& p_node  = pts[path[path.size() - 2].node_index];
          const auto ti_sub   = pts.tp->tokens[p_node.token_begin + 1];
          const auto ti_super = pts.tp->tokens[pts_curr[0].token_begin + 1];
          const auto pts_sub  = pts.sub_tree_span_at(path[path.size() - 2].node_index);
          SILVA_EXPECT(ti_sub != ti_super, MINOR, "{} class can't inherit from itself", pts_sub);
        }
      }
      else if (pts_curr[0].rule_name == lexicon.ni_decl_function) {
        if (is_on_entry(event)) {
          const auto ti = pts.tp->tokens[pts_curr[0].token_begin];
          static_scopes.back().variables.emplace(ti, true);
          static_scopes.push_back(static_scope_t{.class_type = static_scopes.back().class_type});
        }
        if (is_on_exit(event)) {
          static_scopes.pop_back();
        }
      }
      else if (pts_curr[0].rule_name == lexicon.ni_decl_function_param) {
        if (is_on_entry(event)) {
          const auto ti = pts.tp->tokens[pts_curr[0].token_begin];
          static_scopes.back().variables.emplace(ti, true);
        }
      }
      else if (pts_curr[0].rule_name == lexicon.ni_expr_atom) {
        if (is_on_entry(event)) {
          const token_id_t ti = pts.tp->tokens[pts_curr[0].token_begin];
          const auto* tinfo   = pts.tp->token_info_get(pts_curr[0].token_begin);
          const auto pts_ti   = pts.sub_tree_span_at(path.back().node_index);

          const bool is_super = (ti == lexicon.ti_super);
          if (is_super) {
            SILVA_EXPECT(static_scopes.back().class_type == SUBCLASS,
                         MINOR,
                         "{} can only 'super' inside a class that has a superclass",
                         pts_curr);
          }

          const bool is_identifier = tinfo->category == IDENTIFIER;
          const bool is_keyword =
              (ti == lexicon.ti_true || ti == lexicon.ti_false || ti == lexicon.ti_none);
          const name_id_t pr =
              (path.size() >= 2) ? pts[path[path.size() - 2].node_index].rule_name : name_id_root;
          const bool is_member_access =
              (pr == lexicon.ni_expr_member && path.back().child_index == 1);
          const bool is_func_callee = (pr == lexicon.ni_expr_call && path.back().child_index == 0);
          if ((is_identifier && !is_keyword && !is_member_access && !is_func_callee) || is_super) {
            const auto& vars = static_scopes.back().variables;
            if (const auto it = vars.find(ti); it != vars.end()) {
              SILVA_EXPECT(it->second,
                           MINOR,
                           "{} can't read local variable in its own initializer",
                           pts_ti);
            }
            if (const auto dist_val = count_variable(pts_ti, ti)) {
              resolution[pts_ti] = dist_val.value();
            }
          }

          const bool is_number = tinfo->category == NUMBER;
          const bool is_string = tinfo->category == STRING;
          if (is_keyword || is_number || is_string) {
            object_ref_t literal =
                SILVA_EXPECT_FWD_AS(object_ref_from_literal(pts_ti, object_pool, lexicon), MAJOR);
            literals[pts_ti] = std::move(literal);
          }
        }
      }
      return true;
    });
    SILVA_EXPECT_FWD(std::move(result));
    SILVA_EXPECT(static_scopes.size() == 1, ASSERT);

    // for (const auto& [pts_ti, count]: resolution) {
    //   fmt::println("RESOLUTION : {} : {}", to_string(pts_ti), count);
    // }
    // for (const auto& [pts_ti, obj_ref]: literals) {
    //   fmt::println("LITERALS : {} : {}", to_string(pts_ti), to_string(obj_ref));
    // }

    return {};
  }

  struct evaluation_t {
    interpreter_t* intp        = nullptr;
    syntax_ward_ptr_t swp      = intp->lexicon.swp;
    const name_id_style_t& nis = swp->default_name_id_style();
    scope_ptr_t scope;

    expected_t<object_ref_t> call_function(object_ref_t callee, const parse_tree_span_t pts_args)
    {
      function_t* fun = nullptr;
      if (callee->holds_function()) {
        fun = std::get_if<function_t>(&callee->data);
      }
      else if (callee->holds_function_builtin()) {
        fun = std::get_if<function_builtin_t>(&callee->data);
      }
      SILVA_EXPECT(fun, ASSERT);

      const index_t arity = fun->arity();
      SILVA_EXPECT(arity == pts_args[0].num_children,
                   MINOR,
                   "trying to call <function {}>, which has {} parameters, with {} arguments",
                   fun->pts,
                   arity,
                   pts_args[0].num_children);

      const auto fun_params        = fun->parameters();
      auto [args_it, args_end]     = pts_args.children_range();
      auto [params_it, params_end] = fun_params.children_range();
      auto used_scope              = fun->closure.make_child_arm();
      for (index_t i = 0; i < arity; ++i) {
        SILVA_EXPECT(args_it != args_end && params_it != params_end, MAJOR);
        auto val = SILVA_EXPECT_FWD(expr_or_atom(pts_args.sub_tree_span_at(args_it.pos)),
                                    "{} error evaluating {}-th argument (counted 0-based)",
                                    pts_args,
                                    i);
        const token_id_t ti_param =
            fun_params.tp->tokens[fun_params.sub_tree_span_at(params_it.pos)[0].token_begin];
        SILVA_EXPECT_FWD(used_scope.define(ti_param, std::move(val)));
        ++args_it;
        ++params_it;
      }

      SILVA_EXPECT(args_it == args_end && params_it == params_end, MAJOR);
      if (callee->holds_function()) {
        const auto& ff = std::get<function_t>(callee->data);
        auto result    = SILVA_EXPECT_FWD(intp->execute(ff.body(), used_scope));
        return result.value_or(intp->object_pool.make(none));
      }
      else if (callee->holds_function_builtin()) {
        const auto& fb = std::get<function_builtin_t>(callee->data);
        return fb.impl(intp->object_pool, used_scope);
      }
      else {
        SILVA_EXPECT(false, ASSERT);
      }
    }

    expected_t<object_ref_t> expr(const parse_tree_span_t pts)
    {
#define UNARY(op_rule_name, op_func)                                                       \
  else if (rn == op_rule_name)                                                             \
  {                                                                                        \
    const auto [node_idx] = SILVA_EXPECT_FWD(pts.get_children<1>());                       \
    auto res              = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(node_idx)), \
                                "{} error evaluating unary operand",          \
                                pts);                                         \
    return op_func(intp->object_pool, std::move(res));                                     \
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
    return op_func(intp->object_pool, std::move(lhs_res), std::move(rhs_res));        \
  }

      const name_id_t rn = pts[0].rule_name;
      if (false) {
      }
      UNARY(intp->lexicon.ni_expr_u_exc, neg)
      UNARY(intp->lexicon.ni_expr_u_sub, inv)
      BINARY(intp->lexicon.ni_expr_b_mul, mul)
      BINARY(intp->lexicon.ni_expr_b_div, div)
      BINARY(intp->lexicon.ni_expr_b_add, add)
      BINARY(intp->lexicon.ni_expr_b_sub, sub)
      BINARY(intp->lexicon.ni_expr_b_lt, lt)
      BINARY(intp->lexicon.ni_expr_b_gt, gt)
      BINARY(intp->lexicon.ni_expr_b_lte, lte)
      BINARY(intp->lexicon.ni_expr_b_gte, gte)
      BINARY(intp->lexicon.ni_expr_b_eq, eq)
      BINARY(intp->lexicon.ni_expr_b_neq, neq)
      else if (rn == intp->lexicon.ni_expr_b_and)
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
      else if (rn == intp->lexicon.ni_expr_b_or)
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
      else if (rn == intp->lexicon.ni_expr_call)
      {
        const auto [fun_idx, args_idx] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto callee = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(fun_idx)),
                                       "{} error evaluating left-hand-side",
                                       pts);
        SILVA_EXPECT(callee->holds_function() || callee->holds_function_builtin() ||
                         callee->holds_class(),
                     MINOR,
                     "left-hand-side of call-operator must evaluate to function or class");
        const auto pts_args = pts.sub_tree_span_at(args_idx);
        if (callee->holds_function() || callee->holds_function_builtin()) {
          return call_function(callee, pts_args);
        }
        else if (callee->holds_class()) {
          class_t& cc         = std::get<class_t>(callee->data);
          object_ref_t retval = intp->object_pool.make(class_instance_t{._class = callee});
          if (const auto it = cc.methods.find(intp->lexicon.ti_init); it != cc.methods.end()) {
            object_ref_t init_fun_ref = SILVA_EXPECT_FWD(member_bind(retval,
                                                                     {},
                                                                     intp->lexicon.ti_init,
                                                                     intp->object_pool,
                                                                     intp->lexicon.ti_this));
            SILVA_EXPECT(init_fun_ref->holds_function(), MINOR);
            SILVA_EXPECT_FWD(call_function(init_fun_ref, pts_args));
          }
          return retval;
        }
      }
      else if (rn == intp->lexicon.ni_expr_member)
      {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto lhs_ref =
            SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(lhs)),
                             "{} error evaluating left-hand-side of member-access operator",
                             pts);
        const auto pts_rhs = pts.sub_tree_span_at(rhs);
        SILVA_EXPECT(pts_rhs[0].rule_name == intp->lexicon.ni_expr_atom,
                     MAJOR,
                     "{} right-hand-side of member-access operator must be plain identifier",
                     pts_rhs);
        const token_id_t field_name = pts.tp->tokens[pts_rhs[0].token_begin];
        return SILVA_EXPECT_FWD(
            member_get(lhs_ref, field_name, intp->object_pool, intp->lexicon.ti_this),
            "{} could not find {} in {}",
            pts,
            swp->token_id_wrap(field_name),
            pretty_string(lhs_ref));
      }
      else if (rn == intp->lexicon.ni_expr_b_assign)
      {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        auto rhs_ref          = SILVA_EXPECT_FWD(expr_or_atom(pts.sub_tree_span_at(rhs)),
                                        "{} error evaluating right-hand-side of assignment",
                                        pts);
        auto lhs_pts          = pts.sub_tree_span_at(lhs);
        if (lhs_pts[0].rule_name == intp->lexicon.ni_expr_member) {
          const auto [ll, lr] = SILVA_EXPECT_FWD(lhs_pts.get_children<2>());
          auto ll_ref         = SILVA_EXPECT_FWD(expr_or_atom(lhs_pts.sub_tree_span_at(ll)),
                                         "{} error evaluating part of left-hand-side of assignment",
                                         lhs_pts);

          auto lr_pts = lhs_pts.sub_tree_span_at(lr);
          SILVA_EXPECT(lr_pts[0].rule_name == intp->lexicon.ni_expr_atom, MINOR);
          const token_id_t ti = pts.tp->tokens[lr_pts[0].token_begin];
          SILVA_EXPECT(swp->token_infos[ti].category == IDENTIFIER, MINOR);

          SILVA_EXPECT(ll_ref->holds_class_instance(), MINOR);
          auto& ci      = std::get<class_instance_t>(ll_ref->data);
          ci.fields[ti] = rhs_ref;
        }
        else if (lhs_pts[0].rule_name == intp->lexicon.ni_expr_atom) {
          const token_id_t ti = pts.tp->tokens[lhs_pts[0].token_begin];
          SILVA_EXPECT(swp->token_infos[ti].category == IDENTIFIER, MINOR);
          SILVA_EXPECT_FWD(scope.set(ti, rhs_ref), "{} assignment to undeclared variable", pts);
        }
        else {
          SILVA_EXPECT(false, MINOR, "{} unexpected left-hand-side in assignment", lhs_pts);
        }
      }
      else if (rn == intp->lexicon.ni_expr_primary)
      {
        return expr_or_atom(pts.sub_tree_span_at(1));
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
      return intp->object_pool.make(none);
    }

    expected_t<object_ref_t> atom(const parse_tree_span_t pts)
    {
      if (const auto it = intp->literals.find(pts); it != intp->literals.end()) {
        return it->second;
      }
      const token_id_t ti       = pts.tp->tokens[pts[0].token_begin];
      const token_info_t* tinfo = pts.tp->token_info_get(pts[0].token_begin);
      if (ti == intp->lexicon.ti_super) {
        SILVA_EXPECT(pts[0].token_end - pts[0].token_begin == 3, MAJOR);
        const auto it = intp->resolution.find(pts);
        SILVA_EXPECT(it != intp->resolution.end(), MAJOR, "{} could not resolve", pts);
        const index_t dist    = it->second;
        const auto* super_ptr = scope.get_at(intp->lexicon.ti_super, dist);
        SILVA_EXPECT(super_ptr != nullptr, MAJOR);
        const auto* this_ptr = scope.get_at(intp->lexicon.ti_this, dist - 1);
        SILVA_EXPECT(this_ptr != nullptr, MAJOR);
        const token_id_t field_name = pts.tp->tokens[pts[0].token_begin + 2];
        return SILVA_EXPECT_FWD(member_bind(*this_ptr,
                                            *super_ptr,
                                            field_name,
                                            intp->object_pool,
                                            intp->lexicon.ti_this),
                                "{} could not find method {} in superclass",
                                pts,
                                swp->token_id_wrap(field_name));
      }
      else if (tinfo->category == IDENTIFIER) {
        object_ref_t* ptr = [&] {
          const auto it = intp->resolution.find(pts);
          if (it != intp->resolution.end()) {
            return scope.get_at(ti, it->second);
          }
          else {
            return scope.get(ti);
          }
        }();
        SILVA_EXPECT(ptr != nullptr,
                     MINOR,
                     "trying to resolve variable {}",
                     swp->token_id_wrap(ti));
        return *ptr;
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
      if (rule_name == intp->lexicon.ni_expr_atom) {
        return atom(pts);
      }
      else if (swp->name_id_is_parent(intp->lexicon.ni_expr, rule_name)) {
        return expr(pts);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "can't evaluate {}", swp->name_id_wrap(rule_name));
      }
      return intp->object_pool.make(none);
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
    syntax_ward_ptr_t swp = intp->lexicon.swp;

    expected_t<return_t<object_ref_t>> decl(const parse_tree_span_t pts, scope_ptr_t& scope)
    {
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->lexicon.ni_decl_var) {
        const token_id_t var_name = pts.tp->tokens[pts[0].token_begin + 1];
        const auto children       = SILVA_EXPECT_FWD(pts.get_children_up_to<1>());
        object_ref_t initializer  = intp->object_pool.make(none);
        if (children.size == 1) {
          initializer = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(children[0]), scope),
                                         "{} when evaluating initializer of variable declaration",
                                         pts);
        }
        SILVA_EXPECT_FWD(scope.define(var_name, std::move(initializer)),
                         "{} error defining variable {}",
                         pts,
                         swp->token_id_wrap(var_name));
      }
      else if (rule_name == intp->lexicon.ni_decl_fun) {
        const token_id_t fun_name = pts.tp->tokens[pts[0].token_begin + 1];
        SILVA_EXPECT(pts[0].num_children == 1, MAJOR);
        const auto func_pts = pts.sub_tree_span_at(1);
        function_t ff{func_pts};
        ff.closure = scope;
        SILVA_EXPECT_FWD(scope.define(fun_name, intp->object_pool.make(std::move(ff))));
      }
      else if (rule_name == intp->lexicon.ni_decl_class) {
        const token_id_t class_name = pts.tp->tokens[pts[0].token_begin + 1];
        auto [it, end]              = pts.children_range();
        SILVA_EXPECT(it != end, MAJOR);
        const auto pts_super = pts.sub_tree_span_at(it.pos);
        SILVA_EXPECT(pts_super[0].rule_name == intp->lexicon.ni_decl_class_s, MAJOR);
        class_t cc;
        scope_ptr_t used_scope = scope;
        if (pts_super[0].token_begin < pts_super[0].token_end) {
          const auto ti_super              = pts.tp->tokens[pts_super[0].token_begin + 1];
          const object_ref_t* p_superclass = scope.get(ti_super);
          SILVA_EXPECT(p_superclass != nullptr,
                       MINOR,
                       "{} could not find superclass {}",
                       pts,
                       swp->token_id_wrap(ti_super));
          SILVA_EXPECT((*p_superclass)->holds_class(),
                       MINOR,
                       "{} superclass expression does not resolve to a class, but {}",
                       pts,
                       *p_superclass);
          cc.superclass = *p_superclass;
          used_scope    = scope.make_child_arm();
          SILVA_EXPECT_FWD(used_scope.define(intp->lexicon.ti_super, cc.superclass));
        }
        cc.pts = pts;
        ++it;
        while (it != end) {
          const auto pts_method = pts.sub_tree_span_at(it.pos);
          SILVA_EXPECT(pts_method[0].rule_name == intp->lexicon.ni_decl_function, MAJOR);
          const token_id_t method_name = pts.tp->tokens[pts_method[0].token_begin];
          function_t ff{pts_method};
          ff.closure              = used_scope;
          cc.methods[method_name] = intp->object_pool.make(std::move(ff));
          ++it;
        }
        SILVA_EXPECT_FWD(scope.define(class_name, intp->object_pool.make(std::move(cc))));
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown declaration {}", pts, swp->name_id_wrap(rule_name));
      }
      return {std::nullopt};
    }

    expected_t<return_t<object_ref_t>> stmt(const parse_tree_span_t pts, scope_ptr_t& scope)
    {
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == intp->lexicon.ni_stmt_print) {
        object_ref_t value = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(1), scope),
                                              "{} error evaluating argument to 'print'",
                                              pts);
        intp->print_stream->format("{}\n", pretty_string(std::move(value)));
      }
      else if (rule_name == intp->lexicon.ni_stmt_if) {
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
      else if (rule_name == intp->lexicon.ni_stmt_for) {
        const auto [init_idx, cond_idx, inc_idx, body_idx] =
            SILVA_EXPECT_FWD(pts.get_children<4>());
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
      else if (rule_name == intp->lexicon.ni_stmt_while) {
        const auto [cond_idx, body_idx] = SILVA_EXPECT_FWD(pts.get_children<2>());
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
      else if (rule_name == intp->lexicon.ni_stmt_return) {
        if (pts[0].num_children == 1) {
          auto res = SILVA_EXPECT_FWD(intp->evaluate(pts.sub_tree_span_at(1), scope),
                                      "{} error evaluating expression of return statement",
                                      pts);
          return return_t<object_ref_t>{std::move(res)};
        }
        else {
          return return_t<object_ref_t>{intp->object_pool.make(none)};
        }
      }
      else if (rule_name == intp->lexicon.ni_stmt_block) {
        auto old_scope = scope;
        scope_exit_t scope_exit([&] { scope = old_scope; });
        scope = scope.make_child_arm();
        for (const auto [node_idx, child_idx]: pts.children_range()) {
          auto res = SILVA_EXPECT_FWD_PLAIN(go(pts.sub_tree_span_at(node_idx), scope));
          if (res.has_value()) {
            return res;
          }
        }
      }
      else if (rule_name == intp->lexicon.ni_stmt_expr) {
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
      if (rule_name == intp->lexicon.ni_none) {
        ;
      }
      else if (rule_name == intp->lexicon.ni_lox) {
        for (const auto [node_idx, child_idx]: pts.children_range()) {
          auto res = SILVA_EXPECT_FWD_PLAIN(go(pts.sub_tree_span_at(node_idx), scope));
        }
      }
      else if (rule_name == intp->lexicon.ni_decl) {
        return decl(pts.sub_tree_span_at(1), scope);
      }
      else if (swp->name_infos[rule_name].parent_name == intp->lexicon.ni_decl) {
        return decl(pts, scope);
      }
      else if (rule_name == intp->lexicon.ni_stmt) {
        return stmt(pts.sub_tree_span_at(1), scope);
      }
      else if (swp->name_infos[rule_name].parent_name == intp->lexicon.ni_stmt) {
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
