#pragma once

#include "types.hpp"

namespace silva {
  template<typename T>
  struct hybrid_ptr {
    variant_t<const T*, unique_ptr_t<T>> data;

    const T& operator*() const { return *get_pointer(); }

    const T* operator->() const { return get_pointer(); }

    explicit operator bool() const { return get_pointer() != nullptr; }

   private:
    const T* get_pointer() const
    {
      if (std::holds_alternative<const T*>(data)) {
        return std::get<const T*>(data);
      }
      else {
        return std::get<unique_ptr_t<T>>(data).get();
      }
    }
  };
}
