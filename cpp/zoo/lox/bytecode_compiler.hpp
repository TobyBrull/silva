#pragma once

#include "bytecode.hpp"
#include "lox.hpp"

namespace silva::lox {
  struct bytecode_compiler_t {
    lexicon_t lexicon;
    object_pool_t* object_pool = nullptr;

    bytecode_compiler_t(syntax_ward_ptr_t, object_pool_t*);

    expected_t<unique_ptr_t<bytecode_chunk_t>> compile(parse_tree_span_t);
  };
}
