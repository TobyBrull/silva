#pragma once

#include "expected.hpp"
#include "types.hpp"

#include <fmt/format.h>

namespace silva {
  struct byte_source_t {
    span_t<byte_t> span;
    bool is_eof = false;

    static constexpr index_t min_buffer_size = 64;

    virtual void on_read_more(index_t size_hint = 0) = 0;
  };

  struct byte_source_stdin_t : public byte_source_t {
    vector_t<byte_t> buffer;

    byte_source_stdin_t(index_t init_buffer_size = min_buffer_size);
    ~byte_source_stdin_t();

    void on_read_more(index_t = 0) final;
  };

  struct byte_source_memory_t : public byte_source_t {
    vector_t<byte_t> buffer;

    byte_source_memory_t(index_t init_buffer_size = min_buffer_size);

    void on_read_more(index_t = 0) final;
  };
}

// IMPLEMENTATION

namespace silva {
}
