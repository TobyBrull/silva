#pragma once

#include "bytecode.hpp"
#include "lox.hpp"

namespace silva::lox::bytecode {
  struct compiler_t {
    lexicon_t lexicon;

    compiler_t(syntax_ward_ptr_t);

    expected_t<chunk_t> compile(const parse_tree_span_t, object_pool_t&);
  };
}
