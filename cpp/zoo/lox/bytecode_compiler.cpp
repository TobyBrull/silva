#include "bytecode_compiler.hpp"

#include "lox.hpp"
#include "zoo/lox/object.hpp"

namespace silva::lox {

  using enum opcode_t;

  bytecode_compiler_t::bytecode_compiler_t(syntax_farm_ptr_t sfp, object_pool_t* object_pool)
    : lexicon(sfp->get_lexicon<lexicon_t>().ptr()), object_pool(object_pool)
  {
  }

  struct compile_run_t {
    bytecode_compiler_t* compiler = nullptr;

    const lexicon_t& lexicon   = *compiler->lexicon;
    syntax_farm_ptr_t sfp      = lexicon.sfp;
    object_pool_t& object_pool = *compiler->object_pool;

    struct func_scope_t {
      unique_ptr_t<bytecode_chunk_t> chunk;
      bytecode_chunk_nursery_t nursery{.chunk = *chunk};
      array_t<byte_t>& bytecode = nursery.chunk.bytecode;

      index_t scope_depth = 0;
      struct local_t {
        token_id_t var_name;
      };
      array_t<local_t> locals;

      struct upvalue_info_t {
        index_t index = 0;
        bool is_local = false;

        friend auto operator<=>(const upvalue_info_t&, const upvalue_info_t&) = default;
      };
      array_t<upvalue_info_t> upvalue_infos;

      func_scope_t(syntax_farm_ptr_t sfp) : chunk{std::make_unique<bytecode_chunk_t>(sfp)} {}
    };
    array_t<func_scope_t> func_scopes;

    // returns index into "upvalue_infos" of the "func_scopes" entry with index "fs_idx".
    optional_t<index_t> resolve_upvalue(const index_t fs_idx, const token_id_t ti)
    {
      if (fs_idx > 0) {
        if (const auto idx = resolve_local(fs_idx - 1, ti); idx.has_value()) {
          return get_or_create_upvalue(fs_idx - 1, func_scope_t::upvalue_info_t{idx.value(), true});
        }
        if (const auto idx = resolve_upvalue(fs_idx - 1, ti); idx.has_value()) {
          return get_or_create_upvalue(fs_idx - 1,
                                       func_scope_t::upvalue_info_t{idx.value(), false});
        }
      }
      return std::nullopt;
    }
    index_t get_or_create_upvalue(const index_t fs_idx,
                                  const func_scope_t::upvalue_info_t& upvalue_info)
    {
      auto& fs = func_scopes[fs_idx];
      for (index_t i = 0; i < fs.upvalue_infos.size(); ++i) {
        if (fs.upvalue_infos[i] == upvalue_info) {
          return i;
        }
      }
      const index_t retval = fs.upvalue_infos.size();
      fs.upvalue_infos.push_back(upvalue_info);
      return retval;
    }

    struct func_scope_guard_t {
      compile_run_t* compile_run = nullptr;
      func_scope_guard_t(compile_run_t* compile_run) : compile_run(compile_run)
      {
        compile_run->func_scopes.push_back(func_scope_t{compile_run->sfp});
      };
      ~func_scope_guard_t() { compile_run->func_scopes.pop_back(); }
    };
    func_scope_guard_t global_scope_guard{this};
    func_scope_t& cfs() { return func_scopes.back(); }

    struct block_scope_guard_t {
      compile_run_t* compile_run = nullptr;
      parse_tree_span_t pts;
      index_t start_num_locals        = 0;
      index_t start_num_upvalue_infos = 0;
      block_scope_guard_t(compile_run_t* compile_run, const parse_tree_span_t& pts)
        : compile_run(compile_run), pts(pts)
      {
        compile_run->cfs().scope_depth += 1;
        start_num_locals        = compile_run->cfs().locals.size();
        start_num_upvalue_infos = compile_run->cfs().upvalue_infos.size();
      };
      ~block_scope_guard_t()
      {
        array_t<index_t> upvalue_locals;
        while (compile_run->cfs().upvalue_infos.size() > start_num_upvalue_infos) {
          if (compile_run->cfs().upvalue_infos.back().is_local) {
            upvalue_locals.push_back(compile_run->cfs().upvalue_infos.back().index);
          }
          compile_run->cfs().upvalue_infos.pop_back();
        }
        std::ranges::sort(upvalue_locals);
        while (compile_run->cfs().locals.size() > start_num_locals) {
          if (!upvalue_locals.empty() &&
              upvalue_locals.back() + 1 == compile_run->cfs().locals.size()) {
            compile_run->cfs().nursery.append_simple_instr(pts, CLOSE_UPVALUE);
            upvalue_locals.pop_back();
          }
          else {
            compile_run->cfs().nursery.append_simple_instr(pts, POP);
          }
          compile_run->cfs().locals.pop_back();
        }
        compile_run->cfs().scope_depth -= 1;
      }
    };

    compile_run_t(bytecode_compiler_t* compiler) : compiler(compiler) {}

    optional_t<index_t> resolve_local(const index_t fs_idx, const token_id_t var_name)
    {
      const auto& fs  = func_scopes[fs_idx];
      const index_t n = fs.locals.size();
      for (index_t stack_idx = n - 1; stack_idx >= 0; --stack_idx) {
        if (fs.locals[stack_idx].var_name == var_name) {
          return stack_idx;
        }
      }
      return std::nullopt;
    }

    expected_t<void> get_variable(const parse_tree_span_t pts, const token_id_t ti)
    {
      if (const auto idx = resolve_local(func_scopes.size() - 1, ti); idx.has_value()) {
        cfs().nursery.append_index_instr(pts, GET_LOCAL, idx.value());
      }
      else if (const auto idx = resolve_upvalue(func_scopes.size() - 1, ti); idx.has_value()) {
        cfs().nursery.append_index_instr(pts, GET_UPVALUE, idx.value());
      }
      else {
        cfs().nursery.append_index_instr(pts, GET_GLOBAL, ti.val);
      }
      return {};
    }
    expected_t<void> set_variable(const parse_tree_span_t pts, const token_id_t ti)
    {
      if (const auto idx = resolve_local(func_scopes.size() - 1, ti); idx.has_value()) {
        cfs().nursery.append_index_instr(pts, SET_LOCAL, idx.value());
      }
      else if (const auto idx = resolve_upvalue(func_scopes.size() - 1, ti); idx.has_value()) {
        cfs().nursery.append_index_instr(pts, SET_UPVALUE, idx.value());
      }
      else {
        cfs().nursery.append_index_instr(pts, SET_GLOBAL, ti.val);
      }
      return {};
    }

    expected_t<void> expr_atom(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(pts.subtree_size() > 0 && pts.rule_name() == lexicon.ni_expr_atom, ASSERT);
      if (pts.num_children() == 1) {
        const auto [pts_child] = SILVA_EXPECT_FWD(pts.get_children<1>());
        if (pts_child.rule_name() != lexicon.ni_expr_literal &&
            sfp->name_id_is_parent(lexicon.ni_expr, pts_child.rule_name())) {
          return expr(pts_child);
        }
      }
      const auto token = SILVA_EXPECT_FWD(atom_token(pts, lexicon));
      if (token.token_id == lexicon.ti_nil) {
        cfs().nursery.append_simple_instr(pts, NIL);
      }
      else if (token.token_id == lexicon.ti_true) {
        cfs().nursery.append_simple_instr(pts, TRUE);
      }
      else if (token.token_id == lexicon.ti_false) {
        cfs().nursery.append_simple_instr(pts, FALSE);
      }
      else if (token.token_id == lexicon.ti_super) {
        SILVA_EXPECT(token.member_token_id.is_valid(), MAJOR);
        SILVA_EXPECT_FWD(get_variable(pts, lexicon.ti_this));
        SILVA_EXPECT_FWD(get_variable(pts, lexicon.ti_super));
        cfs().nursery.append_index_instr(pts, GET_SUPER, token.member_token_id.val);
      }
      else if (token.category == lexicon.ni_identifier || token.token_id == lexicon.ti_this) {
        SILVA_EXPECT_FWD(get_variable(pts, token.token_id));
      }
      else {
        auto obj_ref = SILVA_EXPECT_FWD(object_ref_from_literal(pts, object_pool, lexicon));
        cfs().nursery.append_constant_instr(pts, std::move(obj_ref));
      }
      return {};
    }

    expected_t<void> expr_unary_prefix(const parse_tree_span_t pts, const opcode_t opcode)
    {
      const auto [pts_operand] = SILVA_EXPECT_FWD(pts.get_children<1>());
      SILVA_EXPECT_FWD(expr(pts_operand));
      cfs().nursery.append_simple_instr(pts, opcode);
      return {};
    }

    expected_t<void> expr_binary(const parse_tree_span_t pts, const opcode_t opcode)
    {
      const auto [pts_lhs, pts_rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
      SILVA_EXPECT_FWD(expr(pts_lhs));
      SILVA_EXPECT_FWD(expr(pts_rhs));
      cfs().nursery.append_simple_instr(pts, opcode);
      return {};
    }

    expected_t<void> expr(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(sfp->name_id_is_parent(lexicon.ni_expr, pts.rule_name()), ASSERT);
      if (pts.rule_name() == lexicon.ni_expr) {
        const auto [pts_child] = SILVA_EXPECT_FWD(pts.get_children<1>());
        return expr(pts_child);
      }
      else if (pts.rule_name() == lexicon.ni_expr_atom) {
        return expr_atom(pts);
      }
      else if (pts.rule_name() == lexicon.ni_expr_u_exc) {
        return expr_unary_prefix(pts, NOT);
      }
      else if (pts.rule_name() == lexicon.ni_expr_u_sub) {
        return expr_unary_prefix(pts, NEGATE);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_mul) {
        return expr_binary(pts, MULTIPLY);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_div) {
        return expr_binary(pts, DIVIDE);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_add) {
        return expr_binary(pts, ADD);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_sub) {
        return expr_binary(pts, SUBTRACT);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_lt) {
        return expr_binary(pts, LESS);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_gt) {
        return expr_binary(pts, GREATER);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_lte) {
        SILVA_EXPECT_FWD(expr_binary(pts, GREATER));
        cfs().nursery.append_simple_instr(pts, NOT);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_gte) {
        SILVA_EXPECT_FWD(expr_binary(pts, LESS));
        cfs().nursery.append_simple_instr(pts, NOT);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_eq) {
        return expr_binary(pts, EQUAL);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_neq) {
        SILVA_EXPECT_FWD(expr_binary(pts, EQUAL));
        cfs().nursery.append_simple_instr(pts, NOT);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_and) {
        const auto [pts_lhs, pts_rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        SILVA_EXPECT_FWD(expr(pts_lhs));
        const index_t j1 = cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0);
        cfs().nursery.append_simple_instr(pts, POP);
        SILVA_EXPECT_FWD(expr(pts_rhs));
        cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_or) {
        const auto [pts_lhs, pts_rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        SILVA_EXPECT_FWD(expr(pts_lhs));
        const index_t j1 = cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0);
        const index_t j2 = cfs().nursery.append_index_instr(pts, JUMP, 0);
        cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1);
        cfs().nursery.append_simple_instr(pts, POP);
        SILVA_EXPECT_FWD(expr(pts_rhs));
        cfs().nursery.backpatch_index_instr(j2, cfs().bytecode.size() - j2);
      }
      else if (pts.rule_name() == lexicon.ni_expr_call) {
        const auto [pts_fun, pts_args] = SILVA_EXPECT_FWD(pts.get_children<2>());
        SILVA_EXPECT_FWD(expr(pts_fun));
        for (const auto pts_arg: pts_args.children_range()) {
          SILVA_EXPECT_FWD(expr(pts_arg));
        }
        cfs().nursery.append_index_instr(pts, CALL, pts_args.num_children());
      }
      else if (pts.rule_name() == lexicon.ni_expr_member) {
        const auto [pts_lhs, pts_rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        SILVA_EXPECT_FWD(expr(pts_lhs));
        SILVA_EXPECT(pts_rhs.rule_name() == lexicon.ni_expr_atom, MINOR);
        const token_id_t field_name = SILVA_EXPECT_FWD(atom_token(pts_rhs, lexicon)).token_id;
        cfs().nursery.append_index_instr(pts, GET_PROPERTY, field_name.val);
      }
      else if (pts.rule_name() == lexicon.ni_expr_b_assign) {
        const auto [lhs_pts, pts_rhs] = SILVA_EXPECT_FWD(pts.get_children<2>());
        SILVA_EXPECT_FWD(expr(pts_rhs), "{} error compiling right-hand-side of assignment", pts);
        if (lhs_pts.rule_name() == lexicon.ni_expr_member) {
          const auto [ll_pts, lr_pts] = SILVA_EXPECT_FWD(lhs_pts.get_children<2>());
          SILVA_EXPECT_FWD(expr(ll_pts));
          SILVA_EXPECT(lr_pts.rule_name() == lexicon.ni_expr_atom, MINOR);
          const auto token = SILVA_EXPECT_FWD(atom_token(lr_pts, lexicon));
          SILVA_EXPECT(token.category == lexicon.ni_identifier, MINOR);
          cfs().nursery.append_index_instr(pts, SET_PROPERTY, token.token_id.val);
        }
        else if (lhs_pts.rule_name() == lexicon.ni_expr_atom) {
          const auto token = SILVA_EXPECT_FWD(atom_token(lhs_pts, lexicon));
          SILVA_EXPECT(token.category == lexicon.ni_identifier, MINOR);
          SILVA_EXPECT_FWD(set_variable(lhs_pts, token.token_id));
        }
        else {
          SILVA_EXPECT(false, MINOR, "{} unexpected left-hand-side in assignment", lhs_pts);
        }
      }
      else {
        SILVA_EXPECT(false,
                     ASSERT,
                     "Not yet implemented: {}",
                     lexicon.name_id_wrap(pts.rule_name()));
      }
      return {};
    }

    expected_t<void> function(const parse_tree_span_t pts, const bool with_this)
    {
      SILVA_EXPECT(pts.rule_name() == lexicon.ni_function, ASSERT);
      function_t fun{pts};
      {
        func_scope_guard_t fsg{this};
        cfs().locals.push_back(func_scope_t::local_t{
            .var_name = with_this ? lexicon.ti_this : token_id_t{},
        });
        const auto pts_fun_p = fun.parameters();
        for (const auto pts_p: pts_fun_p.children_range()) {
          const token_id_t ti_param = SILVA_EXPECT_FWD(pts_p.token());
          cfs().locals.push_back(func_scope_t::local_t{
              .var_name = ti_param,
          });
        }

        SILVA_EXPECT_FWD(go(fun.body()));
        cfs().nursery.append_simple_instr(pts, NIL);
        cfs().nursery.append_simple_instr(pts, RETURN);

        fun.chunk = std::move(cfs().chunk);
      }
      object_ref_t func = object_pool.make(std::move(fun));
      auto& nn          = cfs().nursery;
      const index_t idx = nn.chunk.constant_table.size();
      nn.chunk.constant_table.push_back(std::move(func));
      nn.append(pts, CLOSURE);
      nn.append(pts, idx);
      const auto& upvalue_infos = cfs().upvalue_infos;
      nn.append(pts, index_t(upvalue_infos.size()));
      for (const auto& upvalue_info: upvalue_infos) {
        nn.append(pts, upvalue_info.index);
        nn.append(pts, index_t(upvalue_info.is_local));
      }
      return {};
    }

    expected_t<void> decl(const parse_tree_span_t pts)
    {
      token_id_t decl_name;
      if (pts.rule_name() == lexicon.ni_decl_var) {
        const auto pts_children = SILVA_EXPECT_FWD(pts.get_children_up_to<2>());
        SILVA_EXPECT(pts_children.size >= 1, MINOR);
        decl_name = SILVA_EXPECT_FWD(pts_children[0].token());
        if (pts_children.size == 2) {
          SILVA_EXPECT_FWD(expr(pts_children[1]), "{} error compiling variable initializer", pts);
        }
        else {
          cfs().nursery.append_simple_instr(pts, NIL);
        }
      }
      else if (pts.rule_name() == lexicon.ni_decl_fun) {
        const auto [pts_fun]  = SILVA_EXPECT_FWD(pts.get_children<1>());
        const auto pts_fun_id = SILVA_EXPECT_FWD(pts_fun.iterate_to_child(0));
        decl_name             = SILVA_EXPECT_FWD(pts_fun_id.token());
        SILVA_EXPECT_FWD(function(pts_fun, false));
      }
      else if (pts.rule_name() == lexicon.ni_decl_class) {
        const auto pts_id = SILVA_EXPECT_FWD(pts.iterate_to_child(0));
        decl_name         = SILVA_EXPECT_FWD(pts_id.token());
        cfs().nursery.append_simple_instr(pts, CLASS);
      }
      else {
        SILVA_EXPECT(false,
                     MAJOR,
                     "{} unknown declaration {}",
                     pts,
                     lexicon.name_id_wrap(pts.rule_name()));
      }

      if (func_scopes.size() == 1 && cfs().scope_depth == 0) {
        cfs().nursery.append_index_instr(pts, DEFINE_GLOBAL, decl_name.val);
      }
      else {
        cfs().locals.push_back(func_scope_t::local_t{.var_name = decl_name});
      }

      if (pts.rule_name() == lexicon.ni_decl_class) {
        block_scope_guard_t bsg(this, pts);

        token_id_t superclass_name;

        auto [it, end] = pts.children_range();
        SILVA_EXPECT(it != end, MAJOR);
        ++it;
        SILVA_EXPECT(it != end, MAJOR);

        const auto pts_super = *it;
        SILVA_EXPECT(pts_super.rule_name() == lexicon.ni_decl_class_s, MAJOR);
        if (pts_super.num_children() > 0) {
          const auto [pts_super_id] = SILVA_EXPECT_FWD(pts_super.get_children<1>());
          superclass_name           = SILVA_EXPECT_FWD(pts_super_id.token());
          SILVA_EXPECT_FWD(get_variable(pts_super, superclass_name));
          cfs().locals.push_back(func_scope_t::local_t{.var_name = lexicon.ti_super});
        }
        ++it;

        SILVA_EXPECT_FWD(get_variable(pts, decl_name));
        cfs().locals.push_back(func_scope_t::local_t{});

        if (superclass_name.is_valid()) {
          cfs().nursery.append_simple_instr(pts_super, INHERIT);
        }

        while (it != end) {
          const auto pts_method = *it;
          SILVA_EXPECT_FWD(function(pts_method, true));
          const auto pts_method_id     = SILVA_EXPECT_FWD(pts_method.iterate_to_child(0));
          const token_id_t method_name = SILVA_EXPECT_FWD(pts_method_id.token());
          cfs().nursery.append_index_instr(pts_method, METHOD, method_name.val);
          ++it;
        }
      }

      return {};
    }

    expected_t<void> stmt(const parse_tree_span_t pts)
    {
      const name_id_t rule_name = pts.rule_name();
      if (rule_name == lexicon.ni_stmt_print) {
        SILVA_EXPECT_FWD(expr(pts.subspan_at(1)), "{} error compiling argument to 'print'", pts);
        cfs().nursery.append_simple_instr(pts, PRINT);
      }
      else if (rule_name == lexicon.ni_stmt_if) {
        auto [it, end] = pts.children_range();
        SILVA_EXPECT(it != end, MAJOR);
        SILVA_EXPECT_FWD(expr(*it), "{} error compiling if-condition", pts);
        ++it;
        SILVA_EXPECT(it != end, MAJOR);
        const index_t j1 = cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0);
        cfs().nursery.append_simple_instr(pts, POP);
        SILVA_EXPECT_FWD(go(*it));
        const index_t j2 = cfs().nursery.append_index_instr(pts, JUMP, 0);
        cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1);
        cfs().nursery.append_simple_instr(pts, POP);
        ++it;
        if (it != end) {
          SILVA_EXPECT_FWD(go(*it));
        }
        cfs().nursery.backpatch_index_instr(j2, cfs().bytecode.size() - j2);
      }
      else if (rule_name == lexicon.ni_stmt_for) {
        const auto [pts_init, pts_cond, pts_inc, pts_body] =
            SILVA_EXPECT_FWD(pts.get_children<4>());

        SILVA_EXPECT_FWD(go(pts_init), "{} error compiling 'for' initializer", pts);

        const index_t cond_label = cfs().bytecode.size();
        SILVA_EXPECT_FWD(expr(pts_cond), "{} error compiling 'for' condition", pts);
        const index_t j1 = cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0);
        cfs().nursery.append_simple_instr(pts, POP);
        const index_t j2 = cfs().nursery.append_index_instr(pts, JUMP, 0);

        const index_t inc_label = cfs().bytecode.size();
        SILVA_EXPECT_FWD(expr(pts_inc), "{} error compiling 'for' increment", pts);
        cfs().nursery.append_simple_instr(pts, POP);
        cfs().nursery.append_index_instr(pts, JUMP, cond_label - cfs().bytecode.size());

        cfs().nursery.backpatch_index_instr(j2, cfs().bytecode.size() - j2);
        const index_t body_label = cfs().bytecode.size();
        SILVA_EXPECT_FWD(go(pts_body), "{} error compiling 'for' body", pts);
        cfs().nursery.append_index_instr(pts, JUMP, inc_label - cfs().bytecode.size());

        cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1);
        cfs().nursery.append_simple_instr(pts, POP);
      }
      else if (rule_name == lexicon.ni_stmt_while) {
        const auto [pts_cond, pts_body] = SILVA_EXPECT_FWD(pts.get_children<2>());
        const index_t cond_label        = cfs().bytecode.size();
        SILVA_EXPECT_FWD(expr(pts_cond), "{} error compiling 'while' condition", pts);
        const index_t j1 = cfs().nursery.append_index_instr(pts, JUMP_IF_FALSE, 0);
        cfs().nursery.append_simple_instr(pts, POP);
        SILVA_EXPECT_FWD(go(pts_body));
        cfs().nursery.append_index_instr(pts, JUMP, cond_label - cfs().bytecode.size());
        cfs().nursery.backpatch_index_instr(j1, cfs().bytecode.size() - j1);
        cfs().nursery.append_simple_instr(pts, POP);
      }
      else if (rule_name == lexicon.ni_stmt_return) {
        if (pts.num_children() == 1) {
          SILVA_EXPECT_FWD(expr(pts.subspan_at(1)),
                           "{} error compiling expression of return statement",
                           pts);
        }
        else {
          cfs().nursery.append_simple_instr(pts, NIL);
        }
        cfs().nursery.append_simple_instr(pts, RETURN);
      }
      else if (rule_name == lexicon.ni_stmt_block) {
        block_scope_guard_t bsg{this, pts};
        for (const auto pts_child: pts.children_range()) {
          SILVA_EXPECT_FWD_PLAIN(go(pts_child));
        }
      }
      else if (rule_name == lexicon.ni_stmt_expr) {
        SILVA_EXPECT_FWD(expr(pts.subspan_at(1)), "{} error compiling expression statement", pts);
        cfs().nursery.append_simple_instr(pts, POP);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown statement {}", pts, lexicon.name_id_wrap(rule_name));
      }
      return {};
    }

    expected_t<void> go(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(pts.subtree_size() > 0, MAJOR);
      const name_id_t rule_name = pts.rule_name();
      if (rule_name == lexicon.ni_epsilon) {
        ;
      }
      else if (rule_name == lexicon.ni_lox) {
        for (const auto pts_child: pts.children_range()) {
          SILVA_EXPECT_FWD_PLAIN(go(pts_child));
        }
      }
      else if (rule_name == lexicon.ni_decl) {
        return decl(pts.subspan_at(1));
      }
      else if (sfp->get(rule_name).parent_name == lexicon.ni_decl) {
        return decl(pts);
      }
      else if (rule_name == lexicon.ni_stmt) {
        return stmt(pts.subspan_at(1));
      }
      else if (sfp->get(rule_name).parent_name == lexicon.ni_stmt) {
        return stmt(pts);
      }
      else {
        SILVA_EXPECT(false, MAJOR, "{} unknown rule {}", pts, lexicon.name_id_wrap(rule_name));
      }
      return {};
    }
  };

  expected_t<unique_ptr_t<bytecode_chunk_t>>
  bytecode_compiler_t::compile(const parse_tree_span_t pts)
  {
    compile_run_t run{this};
    SILVA_EXPECT_FWD(run.go(pts));
    SILVA_EXPECT(run.func_scopes.size() == 1, MAJOR);
    return {std::move(run.func_scopes.back().chunk)};
  }
}
