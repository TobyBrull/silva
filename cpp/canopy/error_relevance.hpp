#pragma once

#include "customization_point.hpp"

namespace silva {
  struct error_relevance_t : public customization_point_t<index_t(const void*)> {
    template<typename T>
    constexpr index_t operator()(const T&) const;
  };
  inline constexpr error_relevance_t error_relevance;

  template<typename T>
  index_t error_relevance_impl(const T&);
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  constexpr index_t error_relevance_t::operator()(const T& x) const
  {
    using silva::error_relevance_impl;
    return error_relevance_impl(x);
  }

  template<typename T>
  index_t error_relevance_impl(const T& x)
  {
    return -1;
  }
}
