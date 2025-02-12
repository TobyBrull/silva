#pragma once

#include "assert.hpp"
#include "types.hpp"

#include <atomic>
#include <utility>

namespace silva {
  template<typename T>
  struct menhir_ptr_t;

  class menhir_t {
    mutable std::atomic<index_t> ptr_count = 0;

    template<typename T>
    friend class menhir_ptr_t;

   public:
    menhir_t() = default;
    ~menhir_t();

    menhir_t(menhir_t&&)            = delete;
    menhir_t& operator=(menhir_t&&) = delete;

    menhir_t(const menhir_t&)            = delete;
    menhir_t& operator=(const menhir_t&) = delete;

    template<typename Self>
    auto ptr(this Self&& self)
    {
      return menhir_ptr_t<std::remove_reference_t<Self>>(&self);
    }
  };

  template<typename T>
  class menhir_ptr_t {
    T* ptr = nullptr;

    menhir_ptr_t(T*);

    friend class menhir_t;

    template<typename U>
    friend class menhir_ptr_t;

   public:
    menhir_ptr_t() = default;
    ~menhir_ptr_t();

    menhir_ptr_t(menhir_ptr_t&&);
    menhir_ptr_t(const menhir_ptr_t&);
    menhir_ptr_t& operator=(menhir_ptr_t&&);
    menhir_ptr_t& operator=(const menhir_ptr_t&);

    operator menhir_ptr_t<const T>() const { return menhir_ptr_t<const T>(ptr); }

    bool is_nullptr() const;

    void clear();

    T* operator->() const;
  };
}

// IMPLEMENTATION

namespace silva {
  inline menhir_t::~menhir_t()
  {
    SILVA_ASSERT(ptr_count == 0);
  }

  template<typename T>
  menhir_ptr_t<T>::menhir_ptr_t(T* ptr) : ptr(ptr)
  {
    if (ptr) {
      ptr->ptr_count += 1;
    }
  }

  template<typename T>
  menhir_ptr_t<T>::~menhir_ptr_t()
  {
    if (ptr != nullptr) {
      ptr->ptr_count -= 1;
    }
  }

  template<typename T>
  menhir_ptr_t<T>::menhir_ptr_t(menhir_ptr_t&& other) : ptr(std::exchange(other.ptr, nullptr))
  {
  }

  template<typename T>
  menhir_ptr_t<T>::menhir_ptr_t(const menhir_ptr_t& other) : ptr(other.ptr)
  {
    if (ptr) {
      ptr->ptr_count += 1;
    }
  }

  template<typename T>
  menhir_ptr_t<T>& menhir_ptr_t<T>::operator=(menhir_ptr_t&& other)
  {
    if (this != &other) {
      clear();
      ptr = std::exchange(other.ptr, nullptr);
    }
    return *this;
  }

  template<typename T>
  menhir_ptr_t<T>& menhir_ptr_t<T>::operator=(const menhir_ptr_t& other)
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
  bool menhir_ptr_t<T>::is_nullptr() const
  {
    return ptr == nullptr;
  }

  template<typename T>
  void menhir_ptr_t<T>::clear()
  {
    if (ptr != nullptr) {
      ptr->ptr_count -= 1;
    }
    ptr = nullptr;
  }

  template<typename T>
  T* menhir_ptr_t<T>::operator->() const
  {
    return ptr;
  }
}
