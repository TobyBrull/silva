#pragma once

#include "expected.hpp"
#include "string_or_view.hpp"
#include "types.hpp"

#include <utility>

namespace silva {
  void string_append_escaped(string_t& output_buffer, string_view_t unescaped_string);
  void string_append_unescaped(string_t& output_buffer, string_view_t escaped_string);

  bool string_escape_is_trivial(string_view_t unescaped_string);
  bool string_unescape_is_trivial(string_view_t escaped_string);

  string_t string_escaped(string_view_t unescaped_string);
  string_t string_unescaped(string_view_t escaped_string);

  string_or_view_t string_or_view_escaped(string_view_t unescaped_string);
  string_or_view_t string_or_view_unescaped(string_view_t escaped_string);

  template<typename T>
  expected_t<T> convert_to(string_view_t);
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  expected_t<T> convert_to(string_view_t value)
  {
    if constexpr (std::same_as<bool, T>) {
      if (value == "true") {
        return true;
      }
      else if (value == "false") {
        return false;
      }
      else {
        SILVA_EXPECT(false, MINOR, "Could not convert string '{}' to bool", value);
      }
    }
    else if constexpr (std::same_as<index_t, T>) {
      try {
        return std::stoll(string_t{value});
      }
      catch (...) {
        SILVA_EXPECT(false, MINOR, "Could not convert string '{}' to index_t", value);
      }
    }
    else if constexpr (std::same_as<double, T>) {
      try {
        return std::stod(string_t{value});
      }
      catch (...) {
        SILVA_EXPECT(false, MINOR, "Could not convert string '{}' to double", value);
      }
    }
    else {
      static_assert(false, "Unsupported type in silva::convert_to");
    }
    std::unreachable();
  }
}
