#include "bytecode_compiler.hpp"

#include "syntax/name_id_style.hpp"
#include "zoo/lox/object.hpp"

namespace silva::lox::bytecode {
  compiler_t::compiler_t(syntax_ward_ptr_t swp) : lexicon(std::move(swp)) {}

  struct compile_run_t {
    syntax_ward_ptr_t swp;
    const lexicon_t& lexicon;
    object_pool_t& pool;

    chunk_nursery_t nursery;

    expected_t<void> expr_atom(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(pts[0].rule_name == lexicon.ni_expr_atom, ASSERT);
      auto obj_ref = SILVA_EXPECT_FWD(object_ref_from_literal(pts, pool, lexicon));
      SILVA_EXPECT_FWD(nursery.append_constant(std::move(obj_ref)));
      return {};
    }

    expected_t<void> expr(const parse_tree_span_t pts)
    {
      SILVA_EXPECT(swp->name_id_is_parent(lexicon.ni_expr, pts[0].rule_name), ASSERT);
      if (pts[0].rule_name == lexicon.ni_expr_atom) {
        return expr_atom(pts);
      }
      else {
        SILVA_EXPECT(false, ASSERT, "Not yet implemented");
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
