#pragma once

#include "types.hpp"

namespace silva {
  template<typename Signature>
  struct customization_point_t {
    template<typename T>
    function_t<Signature> any_function(this auto&& self);
  };

  template<typename T>
  concept customization_point_c = std::is_empty_v<T> && std::is_trivial_v<T> && requires(T x) {
    { x.template any_function<int>() };
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename Signature>
  template<typename T>
  function_t<Signature> customization_point_t<Signature>::any_function(this auto&& self)
  {
    using Derived = std::remove_cvref_t<decltype(self)>;
    static_assert(std::is_empty_v<Derived>);
    static_assert(std::is_trivial_v<Derived>);
    return []<typename... Args>(const void* ptr, Args&&... args) {
      return Derived{}(*static_cast<const T*>(ptr), std::forward<Args>(args)...);
    };
  }
}
