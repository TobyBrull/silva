#pragma once

#include "types.hpp"

namespace silva {
  void string_append_escaped(string_t& output_buffer, string_view_t unescaped_string);
  void string_append_unescaped(string_t& output_buffer, string_view_t escaped_string);

  std::string string_escaped(string_view_t unescaped_string);
  std::string string_unescaped(string_view_t escaped_string);
}
