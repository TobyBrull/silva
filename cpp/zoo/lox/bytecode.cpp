#include "bytecode.hpp"

namespace silva::lox::bytecode {

  expected_t<string_t> chunk_t::to_string() const
  {
    string_t retval;
    index_t idx = 0;
    while (true) {
      const auto& [ss, ll] = SILVA_EXPECT_FWD(to_string_at(idx));
      retval += ss;
      retval += '\n';
      idx += ll;
    }
    return retval;
  }
  expected_t<pair_t<string_t, index_t>> chunk_t::to_string_at(const index_t idx) const
  {
    index_t instr_len = 0;
    string_t retval;
    return {{retval, instr_len}};
  }
}
