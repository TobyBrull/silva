#pragma once

#include "bytecode.hpp"

namespace silva::lox::bytecode {
  struct vm_t {
    vector_t<object_ref_t> stack;

    expected_t<object_ref_t> run(const chunk_t&) const;

    expected_t<string_t> to_string() const;
  };
}
