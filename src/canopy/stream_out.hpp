#pragma once

#include "types.hpp"

namespace silva {
  struct stream_out {
    byte_t* current = nullptr;
    byte_t* end     = nullptr;

    virtual void flush(index_t size_hint = 0) = 0;
    virtual void ensure(index_t size)         = 0;
  };

  struct stream_in {
    byte_t* current = nullptr;
    byte_t* end     = nullptr;

    virtual void read(index_t min_size = 0) = 0;
  };
}
