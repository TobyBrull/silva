#pragma once

#include "bytecode.hpp"
#include "lox.hpp"

namespace silva::lox::bytecode {
  struct compiler_t {
    lexicon_t lexicon;
    object_pool_t& object_pool;

    index_t scope_depth = 0;
    struct local_t {
      token_id_t var_name = token_id_none;
      index_t scope_depth = 0;
    };
    vector_t<local_t> locals;

    compiler_t(syntax_ward_ptr_t, object_pool_t&);

    expected_t<void> compile(parse_tree_span_t, chunk_t& target);
  };
}
