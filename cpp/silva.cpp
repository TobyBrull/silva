#include "silva.hpp"

#include "parse_root.hpp"

namespace silva {
  const parse_root_t* silva_parse_root()
  {
    static const parse_root_t retval = SILVA_EXPECT_ASSERT(parse_root_t::create(
        SILVA_EXPECT_ASSERT(token_context_make("silva.seed", string_t{silva_seed}))));
    return &retval;
  }
}
