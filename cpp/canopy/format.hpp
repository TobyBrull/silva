#pragma once

#include "types.hpp"

namespace silva {
  // The first element is interpreted as a format-string
  string_t format_vector(span_t<string_view_t>);
}
