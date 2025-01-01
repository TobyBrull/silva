#include "silva.hpp"

#include "parse_root.hpp"

namespace silva {
  const parse_root_t* silva_parse_root()
  {
    static const parse_root_t retval =
        SILVA_EXPECT_ASSERT(parse_root_t::create(const_ptr_unowned(&silva_seed_source_code)));
    return &retval;
  }
}
