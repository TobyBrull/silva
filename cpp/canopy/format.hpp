#pragma once

#include "types.hpp"

#include <fmt/format.h>

namespace silva {
  string_t format_vector(const string_view_t format, const span_t<string_view_t> args);
}
