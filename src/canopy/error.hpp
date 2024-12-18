#pragma once

#include "context.hpp"
#include "string_or_view.hpp"

namespace silva {

  // For each function that has an "expected_t" return-value, one should always use MINOR errors for
  // the least significant type of error that the function may generate. This should be done even if
  // this type of error is (currently) a higher-level-error in all other contexts. Then map the
  // error-level explicitly, e.g., using one of the SILVA_EXPECT_..._MAP macros.

  enum class error_level_t : uint8_t {
    ALL = 0,

    MINOR = 1,
    MAJOR = 2,
    FATAL = 3,

    ASSERT = 0xfe,

    NONE = 0xff
  };
  constexpr bool error_level_is_primary(error_level_t);

  struct error_context_t : public context_t<error_context_t> {
    constexpr static bool context_use_default = true;
    constexpr static bool context_mutable_get = true;

    vector_t<string_or_view_t> data;
  };

  struct error_t {
    error_level_t level = error_level_t::NONE;
    string_or_view_t message;
    error_context_t* error_context = error_context_t::get();
  };
}

namespace silva {
  constexpr bool error_level_is_primary(const error_level_t error_level)
  {
    switch (error_level) {
      case error_level_t::ALL:
      case error_level_t::NONE:
        return false;

      case error_level_t::MINOR:
      case error_level_t::MAJOR:
      case error_level_t::FATAL:
      case error_level_t::ASSERT:
        return true;
    }
  }
}
