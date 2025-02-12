#pragma once

#include "assert.hpp"
#include "types.hpp"

#include <atomic>
#include <concepts>
#include <utility>

namespace silva {

  template<typename T>
  class ptr_t;

  class sprite_t {
    mutable std::atomic<index_t> ptr_count = 0;

    template<typename T>
    friend class ptr_t;

   public:
    sprite_t() = default;
    ~sprite_t();

    sprite_t(sprite_t&&);
    sprite_t& operator=(sprite_t&&);

    // Sprites are encouraged to implement an explicity "copy" function (they don't have to), but
    // implicit copy is disabled.

    sprite_t(const sprite_t&)            = delete;
    sprite_t& operator=(const sprite_t&) = delete;

    template<typename Self>
    auto ptr(this Self&& self)
    {
      return ptr_t<std::remove_reference_t<Self>>(&self);
    }

    // template<typename Self>
    // auto operator&(this Self&& self)
    // {
    //   static_assert(false, "Use x.ptr() instead of &x");
    // }
  };

  template<typename T>
  class ptr_t {
    T* ptr = nullptr;

    ptr_t(T*);

    friend class sprite_t;

    template<typename U>
    friend class ptr_t;

   public:
    static_assert(std::derived_from<T, sprite_t>);

    ptr_t() = default;
    ~ptr_t();

    ptr_t(ptr_t&&);
    ptr_t(const ptr_t&);
    ptr_t& operator=(ptr_t&&);
    ptr_t& operator=(const ptr_t&);

    operator ptr_t<const T>() const { return ptr_t<const T>(ptr); }

    bool is_nullptr() const;

    void clear();

    T* get() const;
    T* operator->() const;
    T& operator*() const;
  };

  class menhir_t : public sprite_t {
   public:
    menhir_t()  = default;
    ~menhir_t() = default;

    menhir_t(menhir_t&&)            = delete;
    menhir_t& operator=(menhir_t&&) = delete;

    menhir_t(const menhir_t&)            = delete;
    menhir_t& operator=(const menhir_t&) = delete;
  };
}

// IMPLEMENTATION

namespace silva {
  inline sprite_t::~sprite_t()
  {
    SILVA_ASSERT(ptr_count == 0);
  }

  inline sprite_t::sprite_t(sprite_t&&)
  {
    // Init ptr_count = 0.
  }

  inline sprite_t& sprite_t::operator=(sprite_t&&)
  {
    // Keep current ptr_count untouched.
    return *this;
  }

  template<typename T>
  ptr_t<T>::ptr_t(T* ptr) : ptr(ptr)
  {
    if (ptr) {
      ptr->ptr_count += 1;
    }
  }

  template<typename T>
  ptr_t<T>::~ptr_t()
  {
    if (ptr != nullptr) {
      ptr->ptr_count -= 1;
    }
  }

  template<typename T>
  ptr_t<T>::ptr_t(ptr_t&& other) : ptr(std::exchange(other.ptr, nullptr))
  {
  }

  template<typename T>
  ptr_t<T>::ptr_t(const ptr_t& other) : ptr(other.ptr)
  {
    if (ptr) {
      ptr->ptr_count += 1;
    }
  }

  template<typename T>
  ptr_t<T>& ptr_t<T>::operator=(ptr_t&& other)
  {
    if (this != &other) {
      clear();
      ptr = std::exchange(other.ptr, nullptr);
    }
    return *this;
  }

  template<typename T>
  ptr_t<T>& ptr_t<T>::operator=(const ptr_t& other)
  {
    if (this != &other) {
      clear();
      ptr = other.ptr;
      if (ptr) {
        ptr->ptr_count += 1;
      }
    }
    return *this;
  }

  template<typename T>
  bool ptr_t<T>::is_nullptr() const
  {
    return ptr == nullptr;
  }

  template<typename T>
  void ptr_t<T>::clear()
  {
    if (ptr != nullptr) {
      ptr->ptr_count -= 1;
    }
    ptr = nullptr;
  }

  template<typename T>
  T* ptr_t<T>::get() const
  {
    return ptr;
  }

  template<typename T>
  T* ptr_t<T>::operator->() const
  {
    return ptr;
  }

  template<typename T>
  T& ptr_t<T>::operator*() const
  {
    return *ptr;
  }
}
