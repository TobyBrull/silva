#pragma once

#include "string.hpp"

namespace silva {
  // The first element is interpreted as a format-string
  string_t format_vector(span_t<string_view_t>);

  // Convenience wrapper around the above.
  string_t format_vector(span_t<string_t>);
}
