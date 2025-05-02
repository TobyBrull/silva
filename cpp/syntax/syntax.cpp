#include "syntax.hpp"

#include "fern.hpp"

namespace silva {
  unique_ptr_t<seed_engine_t> standard_seed_engine(syntax_ward_ptr_t swp)
  {
    auto retval = std::make_unique<seed_engine_t>(std::move(swp));
    SILVA_EXPECT_ASSERT(retval->add_complete_file("seed.seed", seed_seed));
    SILVA_EXPECT_ASSERT(retval->add_complete_file("seed-axe.seed", seed_axe_seed));
    SILVA_EXPECT_ASSERT(retval->add_complete_file("fern.seed", fern_seed));
    SILVA_EXPECT_ASSERT(retval->add_complete_file("silva.seed", silva_seed));
    return retval;
  }
}
