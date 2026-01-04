#include "string.hpp"

#include <fmt/format.h>

namespace silva {
  string_t hexdump(const string_view_t sv)
  {
    string_t retval;
    for (index_t idx = 0; idx < sv.size(); ++idx) {
      if (idx > 0 && idx % 32 == 0) {
        retval += '\n';
      }
      retval += fmt::format("0x{:02x} ", sv[idx]);
    }
    return retval;
  }

  string_t bindump(const string_view_t sv)
  {
    string_t retval;
    for (index_t idx = 0; idx < sv.size(); ++idx) {
      if (idx > 0 && idx % 8 == 0) {
        retval += '\n';
      }
      retval += fmt::format("0x{:08b} ", sv[idx]);
    }
    return retval;
  }
}
