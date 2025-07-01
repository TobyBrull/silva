#pragma once

#include "bytecode.hpp"

#include "canopy/byte_sink.hpp"

namespace silva::lox::bytecode {
  struct vm_t {
    syntax_ward_ptr_t swp;
    object_pool_t pool;
    vector_t<object_ref_t> stack;
    hash_map_t<token_id_t, object_ref_t> globals;

    byte_sink_t* print_stream = nullptr;

    expected_t<void> run(const chunk_t&);

    expected_t<string_t> to_string() const;
  };
}
