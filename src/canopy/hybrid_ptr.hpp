#pragma once

#include "types.hpp"

namespace silva {
  template<typename T>
  struct hybrid_ptr_t : public sprite_t {
    variant_t<const T*, unique_ptr_t<T>> data;

    const T& operator*() const { return *get(); }
    const T* operator->() const { return get(); }

    explicit operator bool() const { return get() != nullptr; }

    const T* get() const
    {
      if (std::holds_alternative<const T*>(data)) {
        return std::get<const T*>(data);
      }
      else {
        return std::get<unique_ptr_t<T>>(data).get();
      }
    }
  };

  template<typename T>
  hybrid_ptr_t<T> hybrid_ptr_const(const T* ptr)
  {
    hybrid_ptr_t<T> retval;
    retval.data.template emplace<0>(ptr);
    return retval;
  }

  template<typename T>
  hybrid_ptr_t<T> hybrid_ptr_unique(unique_ptr_t<T> ptr)
  {
    hybrid_ptr_t<T> retval;
    retval.data.template emplace<1>(std::move(ptr));
    return retval;
  }
}
