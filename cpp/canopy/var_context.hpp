#pragma once

#include "context.hpp"
#include "convert.hpp"
#include "expected.hpp"
#include "hash.hpp"
#include "string_or_view.hpp"

namespace silva {
  struct var_context_t : public context_t<var_context_t> {
    constexpr static bool context_use_default = true;
    constexpr static bool context_mutable_get = false;

    hashmap_t<string_or_view_t, string_or_view_t> variables;

    var_context_t() = default;
  };

  void var_context_fill_environ(var_context_t*);
  void var_context_fill_cmdline(var_context_t*, int argc, char* argv[]);

  // Errors:
  //  - MINOR: Value doesn't exist.
  expected_t<string_view_t> var_context_get(string_view_t name);

  // Errors:
  //  - MINOR: Value doesn't exist.
  //  - MAJOR: From-string conversion failed.
  template<typename T>
  expected_t<T> var_context_get_as(string_view_t name);
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  expected_t<T> var_context_get_as(const string_view_t name)
  {
    const string_view_t value = SILVA_EXPECT_FWD(var_context_get(name));
    return SILVA_EXPECT_FWD(convert_to<T>(value), MAJOR);
  }
}
