#pragma once

#include "types.hpp"

namespace silva {
  template<typename T>
  struct const_ptr_t {
    variant_t<const T*, std::shared_ptr<const T>> data;

    const_ptr_t() = default;
    const_ptr_t(unique_ptr_t<T> ptr) { data.template emplace<1>(std::move(ptr)); }
    const_ptr_t(unique_ptr_t<const T> ptr) { data.template emplace<1>(std::move(ptr)); }
    const_ptr_t(std::shared_ptr<T> ptr) { data.template emplace<1>(std::move(ptr)); }
    const_ptr_t(std::shared_ptr<const T> ptr) { data.template emplace<1>(std::move(ptr)); }

    const T& operator*() const { return *get(); }
    const T* operator->() const { return get(); }

    explicit operator bool() const { return get() != nullptr; }

    const T* get() const
    {
      if (std::holds_alternative<const T*>(data)) {
        return std::get<const T*>(data);
      }
      else {
        return std::get<std::shared_ptr<const T>>(data).get();
      }
    }
  };

  template<typename T>
  const_ptr_t<T> const_ptr_unowned(const T* ptr)
  {
    const_ptr_t<T> retval;
    retval.data.template emplace<0>(ptr);
    return retval;
  }
}
