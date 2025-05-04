#include "types.hpp"

namespace silva {
  void string_pad(string_t& retval, const index_t size, const char fill_char)
  {
    do {
      retval += fill_char;
    } while (retval.size() < size);
  }
}
