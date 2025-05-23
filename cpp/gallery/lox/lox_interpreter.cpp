#include "lox_interpreter.hpp"

namespace silva::lox {
  expected_t<void> interpreter_t::apply(parse_tree_span_t) const
  {
    return {};
  }
}
