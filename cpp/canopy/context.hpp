#pragma once

#include "types.hpp"

namespace silva {
  template<typename T>
  class context_t : public menhir_t {
    T* previous = nullptr;

    static T* current;
    static T* init_current();
    T* get_pointer();

   protected:
    context_t();
    ~context_t();

   public:
    // In multithreaded applications, each thread should have it's own context, but they may all
    // share a common parent. Thus, only const access is allowed here to
    const T* get_parent() const;

    static T* get()
      requires(T::context_mutable_get);

    static const T* get();
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
  context_t<T>::context_t() : previous(current)
  {
    current = get_pointer();
  }

  template<typename T>
  context_t<T>::~context_t()
  {
    current = previous;
  }

  template<typename T>
  const T* context_t<T>::get_parent() const
  {
    return previous;
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
  {
    if constexpr (T::context_use_default) {
      return current->get_pointer();
    }
    else {
      return current ? current->get_pointer() : nullptr;
    }
  }
}
