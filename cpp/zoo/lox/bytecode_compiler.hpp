#pragma once

#include "bytecode.hpp"

namespace silva::lox::bytecode {
  expected_t<chunk_t> compile(const parse_tree_span_t);
}
