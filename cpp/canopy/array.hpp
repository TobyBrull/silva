#pragma once

#include "types.hpp"

#include <span>
#include <vector>

namespace silva {
  template<typename T>
  struct array_t : public std::vector<T> {
    using std::vector<T>::vector;
    array_t()                          = default;
    array_t(const array_t&)            = default;
    array_t& operator=(const array_t&) = default;
    array_t(array_t&&)                 = default;
    array_t& operator=(array_t&&)      = default;
    ~array_t();
  };

  template<typename T, index_t N>
  using array_fixed_t = std::array<T, N>;

  template<typename T>
  using span_t = std::span<T>;

  template<typename T>
  span_t<T> optional_to_span(optional_t<T>& x);
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  array_t<T>::~array_t()
  {
    while (!this->empty()) {
      this->pop_back();
    }
  }

  template<typename T>
  span_t<T> optional_to_span(optional_t<T>& x)
  {
    if (x.has_value()) {
      return span_t<T>{&x.value(), 1};
    }
    else {
      return {};
    }
  }
}
