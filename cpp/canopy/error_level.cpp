#include "error_level.hpp"

#include "enum.hpp"

namespace silva {
  string_view_t to_string(const error_level_t x)
  {
    static const auto vals = enum_hashmap_to_string<error_level_t>();
    return string_view_t{vals.at(x)};
  }
}
