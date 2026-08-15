#include "lox.hpp"

#include "syntax/syntax.hpp"

namespace silva::lox {
  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t sfp)
  {
    auto retval = standard_seed_interpreter(sfp);
    SILVA_EXPECT_ASSERT(retval->add_seed_text("lox.seed", string_t{seed_str}));
    return retval;
  }

  expected_t<atom_token_t> atom_token(const parse_tree_span_t& pts, const lexicon_t& lexicon)
  {
    SILVA_EXPECT(pts.rule_name() == lexicon.ni_expr_atom, ASSERT);
    const auto [pts_child] = SILVA_EXPECT_FWD(pts.get_children<1>());
    if (pts_child.rule_name() == lexicon.ni_expr_literal) {
      return atom_token_t{
          .token_id = SILVA_EXPECT_FWD(pts_child.token()),
          .category = name_id_literal,
      };
    }
    else if (pts_child.rule_name() == lexicon.ni_identifier &&
             pts_child.fragment_begin() != pts.fragment_begin()) {
      // = "super" '.' identifier
      return atom_token_t{
          .token_id        = lexicon.ti_super,
          .category        = name_id_literal,
          .member_token_id = SILVA_EXPECT_FWD(pts_child.token()),
      };
    }
    else if (pts_child.rule_name() == lexicon.ni_identifier ||
             pts_child.rule_name() == lexicon.ni_number ||
             pts_child.rule_name() == lexicon.ni_string) {
      return atom_token_t{
          .token_id = SILVA_EXPECT_FWD(pts_child.token()),
          .category = pts_child.rule_name(),
      };
    }
    else {
      // = '(' Expr ')'
      return atom_token_t{
          .category = pts_child.rule_name(),
      };
    }
  }
}
