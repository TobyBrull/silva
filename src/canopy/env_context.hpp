#pragma once

#include "context.hpp"
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

  optional_t<string_view_t> env_context_get(string_view_t name);
}
