#include "lox.hpp"

#include "syntax/syntax.hpp"

namespace silva::lox {
  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t sfp)
  {
    auto retval = standard_seed_interpreter(sfp);
    SILVA_EXPECT_ASSERT(retval->add_seed_text("lox.seed", string_t{seed_str}));
    return retval;
  }
}
