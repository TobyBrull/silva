#pragma once

#include "string_or_view.hpp"
#include "types.hpp"

namespace silva {
  void string_append_escaped(string_t& output_buffer, string_view_t unescaped_string);
  void string_append_unescaped(string_t& output_buffer, string_view_t escaped_string);

  bool string_escape_is_trivial(string_view_t unescaped_string);
  bool string_unescape_is_trivial(string_view_t escaped_string);

  string_t string_escaped(string_view_t unescaped_string);
  string_t string_unescaped(string_view_t escaped_string);

  string_or_view_t string_or_view_escaped(string_view_t unescaped_string);
  string_or_view_t string_or_view_unescaped(string_view_t escaped_string);
}
