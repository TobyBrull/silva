#include "silva.hpp"

#include "parse_root.hpp"

namespace silva {
  unique_ptr_t<parse_root_t> silva_parse_root(token_context_ptr_t tcp)
  {
    return SILVA_EXPECT_ASSERT(parse_root_t::create(tcp, "silva.seed", string_t{silva_seed}));
  }
}
