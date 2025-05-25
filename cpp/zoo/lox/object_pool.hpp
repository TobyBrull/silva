#pragma once

#include "canopy/sprite.hpp"
#include "canopy/string_or_view.hpp"

namespace silva {
  template<typename T>
  class object_ref_t;

  template<typename T>
  class object_pool_t : public menhir_t {
    struct item_t {
      index_t ref_count = 0;
      optional_t<T> value;
      index_t next_free = -1;
    };
    vector_t<item_t> items;
    index_t next_free = 0;

    friend class object_ref_t<T>;

   public:
    template<typename... Args>
    object_ref_t<T> make(Args&&...);
  };
  template<typename T>
  using object_pool_ptr_t = ptr_t<object_pool_t<T>>;

  template<typename T>
  class object_ref_t {
    object_pool_ptr_t<T> pool;
    index_t idx = 0;

    friend class object_pool_t<T>;
    object_ref_t(object_pool_ptr_t<T>, index_t);

   public:
    object_ref_t() = default;

    T* operator->() const;
    T& operator*() const;

    friend string_or_view_t to_string_impl(const object_ref_t& x) { return to_string_impl(*x); }
    friend std::ostream& operator<<(std::ostream& os, const object_ref_t& x) { return os << *x; }
  };
}

// IMPLEMENTATION

namespace silva {

  // object_pool_t

  template<typename T>
  template<typename... Args>
  object_ref_t<T> object_pool_t<T>::make(Args&&... args)
  {
    const index_t idx = items.size();
    items.push_back(item_t{
        .ref_count = 1,
        .value     = T{std::forward<Args>(args)...},
        .next_free = -1,
    });
    return object_ref_t<T>{ptr(), idx};
  }

  // object_ref_t

  template<typename T>
  object_ref_t<T>::object_ref_t(object_pool_ptr_t<T> pool, const index_t idx)
    : pool(std::move(pool)), idx(idx)
  {
  }

  template<typename T>
  T* object_ref_t<T>::operator->() const
  {
    return &(pool->items[idx].value.value());
  }
  template<typename T>
  T& object_ref_t<T>::operator*() const
  {
    return pool->items[idx].value.value();
  }
}
