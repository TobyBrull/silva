#pragma once

#include "context.hpp"
#include "string_or_view.hpp"

namespace silva {
  enum class error_level_t {
    NONE     = 0,
    MINOR    = 1,
    MAJOR    = 2,
    CRITICAL = 3,
  };

  struct error_context_t : public context_t<error_context_t> {
    constexpr static bool context_use_default = true;
    constexpr static bool context_mutable_get = true;

    vector_t<string_or_view_t> data;
  };

  struct error_t {
    string_or_view_t message;
    error_context_t* error_context = error_context_t::get();

    error_t(string_or_view_t message) : message(std::move(message)) {}
  };
}
