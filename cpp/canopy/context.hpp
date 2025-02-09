#pragma once

#include "assert.hpp"
#include "types.hpp"

#include <utility>

namespace silva {
  template<typename TConst>
  class context_ptr_t;

  template<typename T>
  class context_t : public menhir_t {
    T* parent         = nullptr;
    index_t ptr_count = 0;

    static T* current;
    static T* init_current();
    T* get_pointer();

    friend class context_ptr_t<T>;
    friend class context_ptr_t<const T>;

   protected:
    context_t();
    ~context_t();

   public:
    context_ptr_t<const T> get_parent() const;

    static context_ptr_t<T> get()
      requires(T::context_mutable_get);

    static context_ptr_t<const T> get()
      requires(!T::context_mutable_get);
  };

  template<typename TConst>
  class context_ptr_t {
    using T = std::remove_const_t<TConst>;
    T* ptr  = nullptr;

    context_ptr_t(T*);

    friend class context_t<T>;

   public:
    context_ptr_t() = default;
    ~context_ptr_t();

    context_ptr_t(context_ptr_t&&);
    context_ptr_t(const context_ptr_t&);
    context_ptr_t& operator=(context_ptr_t&&);
    context_ptr_t& operator=(const context_ptr_t&);

    bool is_nullptr() const;

    void clear();

    TConst* operator->() const;
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  T* context_t<T>::current = context_t<T>::init_current();

  template<typename T>
  T* context_t<T>::init_current()
  {
    static_assert(requires {
      std::derived_from<T, context_t<T>>;
      { T::context_use_default } -> std::convertible_to<bool>;
      { T::context_mutable_get } -> std::convertible_to<bool>;
    });

    if constexpr (T::context_use_default) {
      static T default_context{};
      return &default_context;
    }
    else {
      return nullptr;
    }
  }

  template<typename T>
  T* context_t<T>::get_pointer()
  {
    return static_cast<T*>(this);
  }

  template<typename T>
  context_t<T>::context_t() : parent(current)
  {
    current = get_pointer();
  }

  template<typename T>
  context_t<T>::~context_t()
  {
    current = parent;
    SILVA_ASSERT(ptr_count == 0);
  }

  template<typename T>
  context_ptr_t<const T> context_t<T>::get_parent() const
  {
    return parent;
  }

  template<typename T>
  context_ptr_t<T> context_t<T>::get()
    requires(T::context_mutable_get)
  {
    if constexpr (T::context_use_default) {
      return current->get_pointer();
    }
    else {
      return current ? current->get_pointer() : nullptr;
    }
  }

  template<typename T>
  context_ptr_t<const T> context_t<T>::get()
    requires(!T::context_mutable_get)
  {
    if constexpr (T::context_use_default) {
      return current->get_pointer();
    }
    else {
      return current ? current->get_pointer() : nullptr;
    }
  }

  template<typename TConst>
  context_ptr_t<TConst>::context_ptr_t(T* ptr) : ptr(ptr)
  {
    if (ptr) {
      ptr->ptr_count += 1;
    }
  }

  template<typename TConst>
  context_ptr_t<TConst>::~context_ptr_t()
  {
    if (ptr != nullptr) {
      ptr->ptr_count -= 1;
    }
  }

  template<typename TConst>
  context_ptr_t<TConst>::context_ptr_t(context_ptr_t&& other)
    : ptr(std::exchange(other.ptr, nullptr))
  {
  }

  template<typename TConst>
  context_ptr_t<TConst>::context_ptr_t(const context_ptr_t& other) : ptr(other.ptr)
  {
    if (ptr) {
      ptr->ptr_count += 1;
    }
  }

  template<typename TConst>
  context_ptr_t<TConst>& context_ptr_t<TConst>::operator=(context_ptr_t&& other)
  {
    if (this != &other) {
      clear();
      ptr = std::exchange(other.ptr, nullptr);
    }
    return *this;
  }

  template<typename TConst>
  context_ptr_t<TConst>& context_ptr_t<TConst>::operator=(const context_ptr_t& other)
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

  template<typename TConst>
  bool context_ptr_t<TConst>::is_nullptr() const
  {
    return ptr == nullptr;
  }

  template<typename TConst>
  void context_ptr_t<TConst>::clear()
  {
    if (ptr != nullptr) {
      ptr->ptr_count -= 1;
    }
    ptr = nullptr;
  }

  template<typename TConst>
  TConst* context_ptr_t<TConst>::operator->() const
  {
    return ptr;
  }
}
