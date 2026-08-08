#include "lox.hpp"

#include "syntax/syntax.hpp"

namespace silva::lox {
  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t sfp)
  {
    auto retval = standard_seed_interpreter(sfp);
    SILVA_EXPECT_ASSERT(retval->add_seed_text("lox.seed", string_t{seed_str}));
    return retval;
  }

  expected_t<token_id_t> child_token(const parse_tree_span_t& pts, const index_t child_index)
  {
    const auto pts_child = SILVA_EXPECT_FWD(pts.get_child_by_skipping_pts(child_index));
    return pts_child.token();
  }

  bool is_non_empty(const parse_tree_span_t& pts)
  {
    return pts[0].fragment_begin < pts[0].fragment_end;
  }

  expected_t<atom_token_t> atom_token(const parse_tree_span_t& pts, const lexicon_t& lexicon)
  {
    SILVA_EXPECT(pts[0].rule_name == lexicon.ni_expr_atom, ASSERT);
    if (pts[0].num_children == 0) {
      // A bare keyword: "true" / "false" / "nil" / "this".
      return atom_token_t{
          .token_id = SILVA_EXPECT_FWD(pts.token()),
          .category = name_id_literal,
      };
    }
    const auto pts_child = pts.sub_tree_span_at(1);
    if (pts_child[0].rule_name == lexicon.ni_expr_literal) {
      // A keyword: "true" / "false" / "nil" / "this".
      return atom_token_t{
          .token_id = SILVA_EXPECT_FWD(pts_child.token()),
          .category = name_id_literal,
      };
    }
    if (pts_child[0].rule_name == lexicon.ni_identifier &&
        pts_child[0].fragment_begin != pts[0].fragment_begin) {
      // The "super" '.' identifier form; the atom starts with the "super" keyword.
      return atom_token_t{
          .token_id        = lexicon.ti_super,
          .category        = name_id_literal,
          .member_token_id = SILVA_EXPECT_FWD(pts_child.token()),
      };
    }
    if (pts_child[0].rule_name == lexicon.ni_identifier ||
        pts_child[0].rule_name == lexicon.ni_number ||
        pts_child[0].rule_name == lexicon.ni_string) {
      return atom_token_t{
          .token_id = SILVA_EXPECT_FWD(pts_child.token()),
          .category = pts_child[0].rule_name,
      };
    }
    // The '(' Expr ')' form.
    return atom_token_t{
        .category = pts_child[0].rule_name,
    };
  }
}
