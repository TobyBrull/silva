#include "bytecode_compiler.hpp"

#include "syntax/name_id_style.hpp"
#include "zoo/lox/object.hpp"

namespace silva::lox::bytecode {

  using enum opcode_t;

  compiler_t::compiler_t(syntax_ward_ptr_t swp) : lexicon(std::move(swp)) {}

  struct compile_run_t {
    syntax_ward_ptr_t swp;
    const lexicon_t& lexicon;
    object_pool_t& pool;

    chunk_nursery_t nursery;

    expected_t<void> expr_atom(const parse_tree_span_t pts)
    {
      auto obj_ref = SILVA_EXPECT_FWD(object_ref_from_literal(pts, pool, lexicon));
      nursery.register_origin_info(pts);
      SILVA_EXPECT_FWD(nursery.append_constant_instr(std::move(obj_ref)));
      return {};
    }

    expected_t<void> expr_unary(const parse_tree_span_t pts, const opcode_t opcode)
    {
      SILVA_EXPECT(pts[0].num_children == 1, MAJOR);
      SILVA_EXPECT_FWD(expr(pts.sub_tree_span_at(1)));
      nursery.register_origin_info(pts);
      SILVA_EXPECT_FWD(nursery.append_simple_instr(opcode));
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
      nursery.register_origin_info(pts);
      SILVA_EXPECT_FWD(nursery.append_simple_instr(opcode));
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
      // else if (pts[0].rule_name == lexicon.ni_expr_b_lte) {
      //   return expr_binary(pts, SUBTRACT);
      // }
      // else if (pts[0].rule_name == lexicon.ni_expr_b_gte) {
      //   return expr_binary(pts, SUBTRACT);
      // }
      else if (pts[0].rule_name == lexicon.ni_expr_b_eq) {
        return expr_binary(pts, EQUAL);
      }
      else if (pts[0].rule_name == lexicon.ni_expr_b_neq) {
        SILVA_EXPECT_FWD(expr_binary(pts, EQUAL));
        SILVA_EXPECT_FWD(nursery.append_simple_instr(NOT));
      }
      else {
        SILVA_EXPECT(false, ASSERT, "Not yet implemented: {}", swp->name_id_wrap(pts[0].rule_name));
      }
      return {};
    }
  };

  expected_t<chunk_t> compiler_t::compile(const parse_tree_span_t pts, object_pool_t& pool)
  {
    compile_run_t run{lexicon.swp, lexicon, pool};
    SILVA_EXPECT_FWD(run.expr(pts));
    return {std::move(run.nursery).finish()};
  }
}
