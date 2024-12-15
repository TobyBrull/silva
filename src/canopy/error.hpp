#pragma once

#include "string_or_view.hpp"

namespace silva {
  enum class error_level_t {
    NONE     = 0,
    MINOR    = 1,
    MAJOR    = 2,
    CRITICAL = 3,
  };

  struct error_t {
    string_or_view_t message;

    error_t(string_or_view_t message) : message(std::move(message)) {}
  };
}
