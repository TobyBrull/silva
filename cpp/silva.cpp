#include "silva.hpp"

#include "parse_root.hpp"

namespace silva {
  namespace impl {
    const tokenization_t* silva_seed_tokenization()
    {
      static const tokenization_t* silva_seed_tokenization =
          SILVA_EXPECT_ASSERT(token_context_make("silva.seed", string_t{silva_seed}));
      return silva_seed_tokenization;
    }
  }

  const parse_root_t* silva_parse_root()
  {
    static const parse_root_t retval =
        SILVA_EXPECT_ASSERT(parse_root_t::create(impl::silva_seed_tokenization()));
    return &retval;
  }
}
