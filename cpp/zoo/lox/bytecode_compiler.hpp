#pragma once

#include "bytecode.hpp"

namespace silva::lox::bytecode {
  struct compiler_t {
    expected_t<chunk_t> run(const parse_tree_span_t);
  };
}
