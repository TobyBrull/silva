#include "bytecode_compiler.hpp"

#include "zoo/lox/object.hpp"

using enum silva::token_category_t;

namespace silva::lox::bytecode {

  using enum opcode_t;

  compiler_t::compiler_t(syntax_ward_ptr_t swp, object_pool_t* object_pool)
    : lexicon(std::move(swp)), object_pool(object_pool)
  {
  }

  struct compile_run_t {
    compiler_t* compiler = nullptr;

    const lexicon_t& lexicon   = compiler->lexicon;
    syntax_ward_ptr_t swp      = lexicon.swp;
    object_pool_t& object_pool = *compiler->object_pool;

    index_t scope_depth = 0;
    struct local_t {
      token_id_t var_name = token_id_none;
      index_t scope_depth = 0;
    };
    vector_t<local_t> locals;

    struct func_scope_t {
      index_t locals_idx = 0;
      unique_ptr_t<chunk_t> chunk;
      chunk_nursery_t nursery{.chunk = *chunk};
      vector_t<byte_t>& bytecode = nursery.chunk.bytecode;

      func_scope_t(syntax_ward_ptr_t swp, const index_t locals_idx)
        : locals_idx(locals_idx), chunk{std::make_unique<chunk_t>(swp)}
      {
      }
    };
    vector_t<func_scope_t> func_scopes;
    struct func_scope_guard_t {
      compile_run_t* compile_run = nullptr;
      func_scope_guard_t(compile_run_t* compile_run) : compile_run(compile_run)
      {
        compile_run->scope_depth += 1;
        compile_run->func_scopes.push_back(
            func_scope_t{compile_run->swp, index_t(compile_run->locals.size())});
      };
      ~func_scope_guard_t()
      {
        compile_run->locals.resize(compile_run->func_scopes.back().locals_idx);
        compile_run->func_scopes.pop_back();
        compile_run->scope_depth -= 1;
      }
    };
    func_scope_guard_t global_scope_guard{this};
    func_scope_t& cfs() { return func_scopes.back(); }

    compile_run_t(compiler_t* compiler) : compiler(compiler) {}

    optional_t<index_t> find_local(const token_id_t var_name)
    {
      const index_t n = locals.size();
      for (index_t stack_idx = n - 1; stack_idx >= 0; --stack_idx) {
        if (locals[stack_idx].var_name == var_name) {
          return stack_idx - func_scopes.back().locals_idx;
        }
      }
      return {};
    }

    expected_t<void> expr_atom(const parse_tree_span_t pts)
    {
      const auto ti             = pts.tp->tokens[pts[0].token_begin];
      const token_info_t* tinfo = pts.tp->token_info_get(pts[0].token_begin);
      if (ti == lexicon.ti_none) {
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, NIL));
      }
      else if (ti == lexicon.ti_true) {
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, TRUE));
      }
      else if (ti == lexicon.ti_false) {
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, FALSE));
      }
      else if (tinfo->category == IDENTIFIER) {
        const auto fl = find_local(ti);
        if (fl.has_value() && fl.value() >= 0) {
          SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, GET_LOCAL, fl.value()));
        }
        else {
          SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, GET_GLOBAL, ti));
        }
      }
      else {
        auto obj_ref = SILVA_EXPECT_FWD(object_ref_from_literal(pts, object_pool, lexicon));
        SILVA_EXPECT_FWD(cfs().nursery.append_constant_instr(pts, std::move(obj_ref)));
      }
      return {};
    }

    expected_t<void> expr_unary(const parse_tree_span_t pts, const opcode_t opcode)
    {
      SILVA_EXPECT(pts[0].num_children == 1, MAJOR);
      SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(1)));
      SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, opcode));
      return {};
    }

    expected_t<void> expr_binary(const parse_tree_span_t pts, const opcode_t opcode)
    {
      SILVA_EXPECT(pts[0].num_children == 2, MAJOR);
      auto [it, end] = pts.children_range();
      SILVA_EXPECT(it != end, MAJOR);
      SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(it.pos)));
      ++it;
      SILVA_EXPECT(it != end, MAJOR);
      SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(it.pos)));
      SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, opcode));
      return {};
    }

    expected_t<void> expr(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(swp->name_id_is_parent(lexicon.ni_expr, pts[0].rule_name), ASSERT);
      if (pts[0].rule_name == lexicon.ni_expr_atom) {
        return expr_atom(pts);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_primary) {
        return expr(pts.sub_tree_span_at(1));
      }
      else if (pts[0].rule_name == lexicon.ni_expr_u_exc) {
        return expr_unary(pts, NOT);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_u_sub) {
        return expr_unary(pts, NEGATE);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_mul) {
        return expr_binary(pts, MULTIPLY);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_div) {
        return expr_binary(pts, DIVIDE);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_add) {
        return expr_binary(pts, ADD);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_sub) {
        return expr_binary(pts, SUBTRACT);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_lt) {
        return expr_binary(pts, LESS);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_gt) {
        return expr_binary(pts, GREATER);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_lte) {
        SILVA_EXPECT_FWD(expr_binary(pts, GREATER));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, NOT));
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_gte) {
        SILVA_EXPECT_FWD(expr_binary(pts, LESS));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, NOT));
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_eq) {
        return expr_binary(pts, EQUAL);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_neq) {
        SILVA_EXPECT_FWD(expr_binary(pts, EQUAL));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, NOT));
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_and) {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(lhs)));
        const index_t j1 =
            SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(rhs)));
        SILVA_EXPECT_FWD(cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1));
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_or) {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(lhs)));
        const index_t j1 =
            SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0));
        const index_t j2 = SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, JUMP, 0));
        SILVA_EXPECT_FWD(cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(rhs)));
        SILVA_EXPECT_FWD(cfs().nursery.backpatch_index_instr(j2, cfs().bytecode.size() - j2));
      }
      else if (pts[0].rule_name == lexicon.ni_expr_call) {
        const auto [fun_idx, args_idx] = SILVA_EXPECT_FWD(pts.get_children<2>());
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(fun_idx)));
        const auto pts_args = pts.sub_tree_span_at(args_idx);
        for (const auto [node_idx, child_idx]: pts_args.children_range()) {
          SILVA_EXPECT_FWD(expr(pts_args.sub_tree_span_at(node_idx)));
        }
        SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, CALL, pts_args[0].num_children));
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_assign) {
        const auto [lhs, rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(rhs)),
                         "{} error compiling right-hand-side of assignment",
                         pts);
        auto lhs_pts = pts.sub_tree_span_at(lhs);
        if (lhs_pts[0].rule_name == lexicon.ni_expr_member) {
          SILVA_EXPECT(false, ASSERT, "not implemented yet");

          const auto [ll, lr] = SILVA_EXPECT_FWD(lhs_pts.get_children<2>());
          SILVA_EXPECT_FWD(expr(lhs_pts.sub_tree_span_at(ll)),
                           "{} error compiling part of left-hand-side of assignment",
                           lhs_pts);
          auto lr_pts = lhs_pts.sub_tree_span_at(lr);
          SILVA_EXPECT(lr_pts[0].rule_name == lexicon.ni_expr_atom, MINOR);
          const token_id_t ti = pts.tp->tokens[lr_pts[0].token_begin];
          SILVA_EXPECT(swp->token_infos[ti].category == IDENTIFIER, MINOR);

          // SILVA_EXPECT(ll_ref->holds_class_instance(), MINOR);
          // auto& ci      = std::get<class_instance_t>(ll_ref->data);
          // ci.fields[ti] = rhs_ref;
        }
        else if (lhs_pts[0].rule_name == lexicon.ni_expr_atom) {
          const token_id_t ti = pts.tp->tokens[lhs_pts[0].token_begin];
          SILVA_EXPECT(swp->token_infos[ti].category == IDENTIFIER, MINOR);
          const auto fl = find_local(ti);
          if (fl.has_value()) {
            SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, SET_LOCAL, fl.value()));
          }
          else {
            SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, SET_GLOBAL, ti));
          }
        }
        else {
          SILVA_EXPECT(false, MINOR, "{} unexpected left-hand-side in assignment", lhs_pts);
        }
      }
      else {
        SILVA_EXPECT(false, ASSERT, "Not yet implemented: {}", swp->name_id_wrap(pts[0].rule_name));
      }
      return {};
    }

    expected_t<void> decl(const parse_tree_span_t pts)
    {
      const name_id_t rule_name  = pts[0].rule_name;
      const token_id_t decl_name = pts.tp->tokens[pts[0].token_begin + 1];

      if (pts[0].rule_name == lexicon.ni_decl_var) {
        const auto children = SILVA_EXPECT_FWD(pts.get_children_up_to<1>());
        if (children.size == 1) {
          SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(children[0])),
                           "{} error compiling variable initializer",
                           pts);
        }
        else {
          SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, NIL));
        }
      }
      else if (rule_name == lexicon.ni_decl_fun) {
        SILVA_EXPECT(pts[0].num_children == 1, MAJOR);
        const auto func_pts = pts.sub_tree_span_at(1);
        function_t fun{func_pts};
        {
          func_scope_guard_t fsg{this};
          locals.push_back(local_t{
              .var_name    = token_id_none,
              .scope_depth = scope_depth,
          });
          const auto pts_fun_p = fun.parameters();
          for (const auto [node_idx, child_idx]: pts_fun_p.children_range()) {
            const auto pts_p          = pts_fun_p.sub_tree_span_at(node_idx);
            const token_id_t ti_param = pts_p.tp->tokens[pts_p[0].token_begin];
            locals.push_back(local_t{
                .var_name    = ti_param,
                .scope_depth = scope_depth,
            });
          }

          SILVA_EXPECT_FWD(go(fun.body()));
          SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(func_pts, NIL));
          SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(func_pts, RETURN));

          fun.chunk = std::move(cfs().chunk);
        }
        object_ref_t func = object_pool.make(std::move(fun));
        SILVA_EXPECT_FWD(cfs().nursery.append_constant_instr(pts, std::move(func)));
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown declaration {}", pts, swp->name_id_wrap(rule_name));
      }

      if (scope_depth == 1) {
        SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, DEFINE_GLOBAL, decl_name));
      }
      else {
        locals.push_back(local_t{
            .var_name    = decl_name,
            .scope_depth = scope_depth,
        });
      }
      return {};
    }

    expected_t<void> stmt(const parse_tree_span_t pts)
    {
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == lexicon.ni_stmt_print) {
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(1)),
                         "{} error compiling argument to 'print'",
                         pts);
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, PRINT));
      }
      else if (rule_name == lexicon.ni_stmt_if) {
        auto [it, end] = pts.children_range();
        SILVA_EXPECT(it != end, MAJOR);
        pts.sub_tree_span_at(it.pos);
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(it.pos)),
                         "{} error compiling if-condition",
                         pts);
        ++it;
        SILVA_EXPECT(it != end, MAJOR);
        const index_t j1 =
            SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
        SILVA_EXPECT_FWD(go(pts.sub_tree_span_at(it.pos)));
        const index_t j2 = SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, JUMP, 0));
        SILVA_EXPECT_FWD(cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
        ++it;
        if (it != end) {
          SILVA_EXPECT_FWD(go(pts.sub_tree_span_at(it.pos)));
        }
        SILVA_EXPECT_FWD(cfs().nursery.backpatch_index_instr(j2, cfs().bytecode.size() - j2));
      }
      else if (rule_name == lexicon.ni_stmt_for) {
        const auto [init_idx, cond_idx, inc_idx, body_idx] =
            SILVA_EXPECT_FWD(pts.get_children<4>());

        SILVA_EXPECT_FWD(go(pts.sub_tree_span_at(init_idx)),
                         "{} error compiling 'for' initializer",
                         pts);

        const index_t cond_label = cfs().bytecode.size();
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(cond_idx)),
                         "{} error compiling 'for' condition",
                         pts);
        const index_t j1 =
            SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
        const index_t j2 = SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, JUMP, 0));

        const index_t inc_label = cfs().bytecode.size();
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(inc_idx)),
                         "{} error compiling 'for' increment",
                         pts);
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
        SILVA_EXPECT_FWD(
            cfs().nursery.append_index_instr(pts, JUMP, cond_label - cfs().bytecode.size()));

        SILVA_EXPECT_FWD(cfs().nursery.backpatch_index_instr(j2, cfs().bytecode.size() - j2));
        const index_t body_label = cfs().bytecode.size();
        SILVA_EXPECT_FWD(go(pts.sub_tree_span_at(body_idx)), "{} error compiling 'for' body", pts);
        SILVA_EXPECT_FWD(
            cfs().nursery.append_index_instr(pts, JUMP, inc_label - cfs().bytecode.size()));

        SILVA_EXPECT_FWD(cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
      }
      else if (rule_name == lexicon.ni_stmt_while) {
        const auto [cond_idx, body_idx] = SILVA_EXPECT_FWD(pts.get_children<2>());
        const index_t cond_label        = cfs().bytecode.size();
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(cond_idx)),
                         "{} error compiling 'while' condition",
                         pts);
        const index_t j1 =
            SILVA_EXPECT_FWD(cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
        SILVA_EXPECT_FWD(go(pts.sub_tree_span_at(body_idx)));
        SILVA_EXPECT_FWD(
            cfs().nursery.append_index_instr(pts, JUMP, cond_label - cfs().bytecode.size()));
        SILVA_EXPECT_FWD(cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1));
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
      }
      else if (rule_name == lexicon.ni_stmt_return) {
        if (pts[0].num_children == 1) {
          SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(1)),
                           "{} error compiling expression of return statement",
                           pts);
        }
        else {
          SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, NIL));
        }
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, RETURN));
      }
      else if (rule_name == lexicon.ni_stmt_block) {
        scope_depth += 1;
        for (const auto [node_idx, child_idx]: pts.children_range()) {
          SILVA_EXPECT_FWD_PLAIN(go(pts.sub_tree_span_at(node_idx)));
        }
        while (!locals.empty() && locals.back().scope_depth == scope_depth) {
          locals.pop_back();
          SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
        }
        scope_depth -= 1;
      }
      else if (rule_name == lexicon.ni_stmt_expr) {
        SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(1)),
                         "{} error compiling expression statement",
                         pts);
        SILVA_EXPECT_FWD(cfs().nursery.append_simple_instr(pts, POP));
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown statement {}", pts, swp->name_id_wrap(rule_name));
      }
      return {};
    }

    expected_t<void> go(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(pts.size() > 0, MAJOR);
      const name_id_t rule_name = pts[0].rule_name;
      if (rule_name == lexicon.ni_none) {
        ;
      }
      else if (rule_name == lexicon.ni_lox) {
        for (const auto [node_idx, child_idx]: pts.children_range()) {
          SILVA_EXPECT_FWD_PLAIN(go(pts.sub_tree_span_at(node_idx)));
        }
      }
      else if (rule_name == lexicon.ni_decl) {
        return decl(pts.sub_tree_span_at(1));
      }
      else if (swp->name_infos[rule_name].parent_name == lexicon.ni_decl) {
        return decl(pts);
      }
      else if (rule_name == lexicon.ni_stmt) {
        return stmt(pts.sub_tree_span_at(1));
      }
      else if (swp->name_infos[rule_name].parent_name == lexicon.ni_stmt) {
        return stmt(pts);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown rule {}", pts, swp->name_id_wrap(rule_name));
      }
      return {};
    }
  };

  expected_t<unique_ptr_t<chunk_t>> compiler_t::compile(const parse_tree_span_t pts)
  {
    compile_run_t run{this};
    SILVA_EXPECT_FWD(run.go(pts));
    SILVA_EXPECT(run.func_scopes.size() == 1, MAJOR);
    return {std::move(run.func_scopes.back().chunk)};
  }
}
