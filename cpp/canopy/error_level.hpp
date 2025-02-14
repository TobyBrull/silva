#pragma once

#include "types.hpp"

namespace silva {

  // For each function that has an "expected_t" return-value, one should always use MINOR errors for
  // the least significant type of error that the function may generate. This should be done even if
  // this type of error is considered of higher level in all other current contexts. The error-level
  // should then be mapped as required, e.g., via "SILVA_EXPECT_FWD(..., MAJOR)".

  enum class error_level_t : uint8_t {
    NO_ERROR = 0,
    MINOR    = 1,
    MAJOR    = 2,
    FATAL    = 3,
    ASSERT   = 4,
  };
  constexpr bool error_level_is_primary(error_level_t);
  string_view_t to_string(error_level_t);
}

// IMPLEMENTATION

namespace silva {
  constexpr bool error_level_is_primary(const error_level_t error_level)
  {
    switch (error_level) {
      case error_level_t::NO_ERROR:
        return false;

      case error_level_t::MINOR:
      case error_level_t::MAJOR:
      case error_level_t::FATAL:
      case error_level_t::ASSERT:
        return true;
    }
  }
}
