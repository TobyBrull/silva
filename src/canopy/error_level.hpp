#pragma once

#include <cstdint>

namespace silva {

  // For each function that has an "expected_t" return-value, one should always use MINOR errors for
  // the least significant type of error that the function may generate. This should be done even if
  // this type of error is considered of higher level in all other current contexts. The error-level
  // should then be mapped as required, e.g., via "SILVA_EXPECT_FWD(..., MAJOR)".

  enum class error_level_t : uint8_t {
    NO_ERROR = 0,
    RUNTIME  = 1,
    MINOR    = 2,
    MAJOR    = 3,

    // Errors that should never if the currently handled parse_tree(_span)_t was parsed according to
    // the required Seed program.
    BROKEN_SEED = 4,

    FATAL  = 5,
    ASSERT = 6,
  };
  constexpr bool error_level_is_primary(error_level_t);
}

// IMPLEMENTATION

namespace silva {
  constexpr bool error_level_is_primary(const error_level_t error_level)
  {
    using enum error_level_t;

    switch (error_level) {
      case NO_ERROR:
        return false;

      case RUNTIME:
      case MINOR:
      case MAJOR:
      case BROKEN_SEED:
      case FATAL:
      case ASSERT:
        return true;
    }
  }
}
