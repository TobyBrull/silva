#include "lox.hpp"

#include "syntax/syntax.hpp"

namespace silva::lox {
  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_ward_ptr_t swp)
  {
    auto retval = standard_seed_interpreter(swp);
    SILVA_EXPECT_ASSERT(retval->add_complete_file("lox.seed", seed_str));
    return retval;
  }
}
