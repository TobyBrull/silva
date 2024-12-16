#pragma once

#include "types.hpp"

namespace silva {
  template<typename T>
  class context_t : public menhir_t {
    context_t<T>* previous = nullptr;

    static context_t<T>* current;
    static context_t* init_current();
    T* get_pointer();

   protected:
    context_t();
    ~context_t();

   public:
    static T* get()
      requires(T::context_mutable_get);

    static const T* get()
      requires(!T::context_mutable_get);
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  context_t<T>* context_t<T>::current = context_t<T>::init_current();

  template<typename T>
  context_t<T>* context_t<T>::init_current()
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
  context_t<T>::context_t() : previous(current)
  {
    current = this;
  }

  template<typename T>
  context_t<T>::~context_t()
  {
    current = previous;
  }

  template<typename T>
  T* context_t<T>::get()
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
  const T* context_t<T>::get()
    requires(!T::context_mutable_get)
  {
    if constexpr (T::context_use_default) {
      return current->get_pointer();
    }
    else {
      return current ? current->get_pointer() : nullptr;
    }
  }
}
