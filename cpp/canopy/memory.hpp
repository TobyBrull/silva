#pragma once

#include "customization_point.hpp"

namespace silva {
  struct move_ctor_t : public customization_point_t<void(void*, byte_t*)> {
    template<typename T>
    constexpr void operator()(T& src, byte_t* tgt) const;
  };
  inline constexpr move_ctor_t move_ctor;

  struct dtor_t : public customization_point_t<void(void*)> {
    template<typename T>
    constexpr void operator()(T& tgt) const;
  };
  inline constexpr dtor_t dtor;
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  constexpr void move_ctor_t::operator()(T& src, byte_t* tgt) const
  {
    new (tgt) T(std::move(src));
  }

  template<typename T>
  constexpr void dtor_t::operator()(T& tgt) const
  {
    tgt.~T();
  }
}
