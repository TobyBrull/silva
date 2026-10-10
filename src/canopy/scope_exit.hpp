#pragma once

#include <utility>

namespace silva {
  template<typename ExitFunc>
  struct scope_exit_t {
    ExitFunc exit_func;

    ~scope_exit_t();

    explicit scope_exit_t(ExitFunc&& exit_func);

    scope_exit_t(scope_exit_t&& other)           = delete;
    scope_exit_t(const scope_exit_t&)            = delete;
    scope_exit_t& operator=(const scope_exit_t&) = delete;
    scope_exit_t& operator=(scope_exit_t&&)      = delete;
  };

  template<typename ExitFunc>
  scope_exit_t(ExitFunc&&) -> scope_exit_t<std::decay_t<ExitFunc>>;
}

// IMPLEMENTATION

namespace silva {
  template<typename ExitFunc>
  scope_exit_t<ExitFunc>::~scope_exit_t()
  {
    exit_func();
  }

  template<typename ExitFunc>
  scope_exit_t<ExitFunc>::scope_exit_t(ExitFunc&& exit_func)
    : exit_func(std::forward<ExitFunc>(exit_func))
  {
  }
}
