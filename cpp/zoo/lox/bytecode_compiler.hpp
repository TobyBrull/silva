#pragma once

#include "bytecode.hpp"
#include "lox.hpp"

namespace silva::lox::bytecode {
  struct compiler_t {
    lexicon_t lexicon;
    object_pool_t& object_pool;

    compiler_t(syntax_ward_ptr_t, object_pool_t&);

    expected_t<unique_ptr_t<chunk_t>> compile(const parse_tree_span_t);
  };
}
