#pragma once

#include "types.hpp"

namespace silva {
  constexpr index_t chunked_size(index_t size, index_t chunk_size);
}

// IMPLEMENTATION

namespace silva {
  constexpr index_t chunked_size(const index_t size, const index_t chunk_size)
  {
    if (size == 0) {
      return 0;
    }
    return (((size - 1) / chunk_size) + 1) * chunk_size;
  }
}
