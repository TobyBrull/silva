#include "silva.hpp"

#include "parse_root.hpp"

namespace silva {
  const parse_root_t* silva_parse_root()
  {
    static const tokenization_t retval_tt =
        SILVA_EXPECT_ASSERT(token_context_make("silva.seed", string_t{silva_seed}));
    static const parse_tree_t retval_pt = SILVA_EXPECT_ASSERT(seed_parse(retval_tt.ptr()));
    static const parse_root_t retval = SILVA_EXPECT_ASSERT(parse_root_t::create(retval_pt.ptr()));
    return &retval;
  }
}
