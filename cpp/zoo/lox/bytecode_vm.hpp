#pragma once

#include "bytecode.hpp"

#include "canopy/byte_sink.hpp"

namespace silva::lox::bytecode {
  struct vm_t {
    object_pool_t pool;
    vector_t<object_ref_t> stack;

    byte_sink_t* print_stream = nullptr;

    expected_t<void> run(const chunk_t&);

    expected_t<string_t> to_string() const;
  };
}
