#pragma once

#include "context.hpp"
#include "convert.hpp"
#include "expected.hpp"
#include "string_or_view.hpp"

namespace silva {
  struct env_context_t : public context_t<env_context_t> {
    constexpr static bool context_use_default = true;
    constexpr static bool context_mutable_get = false;

    hashmap_t<string_or_view_t, string_or_view_t> variables;

    env_context_t() = default;
  };

  void env_context_fill_environ(env_context_t*);
  void env_context_fill_cmdline(env_context_t*, int argc, char* argv[]);

  // Errors:
  //  - MINOR: Value doesn't exist.
  expected_t<string_view_t> env_context_get(string_view_t name);

  // Errors:
  //  - MINOR: Value doesn't exist.
  //  - MAJOR: From-string conversion failed.
  template<typename T>
  expected_t<T> env_context_get_as(string_view_t name);
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  expected_t<T> env_context_get_as(const string_view_t name)
  {
    const string_view_t value = SILVA_EXPECT_FWD(env_context_get(name));
    return SILVA_EXPECT_FWD_WITH_AT_LEAST(convert_to<T>(value), MAJOR);
  }
}
