#pragma once

#include "types.hpp"

namespace silva {
  template<typename T>
  void bit_append(string_t& buffer, T x);

  template<typename T>
  T bit_cast_ptr(const void* ptr);

  template<typename T>
  void bit_write_at(void* ptr, T x);
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  void bit_append(string_t& buffer, T x)
  {
    const index_t old_size = buffer.size();
    buffer.resize(old_size + sizeof(T));
    memcpy(buffer.data() + old_size, &x, sizeof(T));
  }

  template<typename T>
  T bit_cast_ptr(const void* ptr)
  {
    T retval;
    memcpy(&retval, ptr, sizeof(T));
    return retval;
  }

  template<typename T>
  void bit_write_at(void* ptr, T x)
  {
    memcpy(ptr, &x, sizeof(T));
  }
}
