#pragma once

#include "bytecode.hpp"

namespace silva::lox::bytecode {
  struct vm_t {
    object_pool_t pool;
    vector_t<object_ref_t> stack;

    expected_t<void> run(const chunk_t&);

    expected_t<string_t> to_string() const;
  };
}
