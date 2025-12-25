#pragma once

#include "types.hpp"

#include <string>

namespace silva {
  using string_t = std::string;
  void string_pad(string_t&, index_t size, char fill_char = ' ');

  using string_view_t = std::string_view;
}
