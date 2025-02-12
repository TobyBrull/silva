#pragma once

#include "sprite.hpp"

namespace silva {
  template<typename T>
  class context_t : public menhir_t {
    T* parent = nullptr;

    static T* current;
    static T* init_current();
    T* get_pointer();

   protected:
    context_t();
    ~context_t();

   public:
    ptr_t<const T> get_parent() const;

    static ptr_t<T> get()
      requires(T::context_mutable_get);

    static ptr_t<const T> get()
      requires(!T::context_mutable_get);
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
  }

  template<typename T>
  ptr_t<const T> context_t<T>::get_parent() const
  {
    if (parent) {
      return parent->ptr();
    }
    else {
      return {};
    }
  }

  template<typename T>
  ptr_t<T> context_t<T>::get()
    requires(T::context_mutable_get)
  {
    if constexpr (T::context_use_default) {
      return current->get_pointer()->ptr();
    }
    else {
      return current ? current->get_pointer()->ptr() : nullptr;
    }
  }

  template<typename T>
  ptr_t<const T> context_t<T>::get()
    requires(!T::context_mutable_get)
  {
    if constexpr (T::context_use_default) {
      return current->get_pointer()->ptr();
    }
    else {
      return current ? current->get_pointer()->ptr() : nullptr;
    }
  }
}
