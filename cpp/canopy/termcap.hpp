#pragma once

#include "string.hpp"

namespace silva::termcap {
  inline constexpr string_view_t bold  = "\033[1m";
  inline constexpr string_view_t red   = "\033[31m";
  inline constexpr string_view_t green = "\033[32m";
  inline constexpr string_view_t reset = "\033[0m";
}
