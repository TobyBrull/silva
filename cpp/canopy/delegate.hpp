#pragma once

#include <utility>

namespace silva {
  template<typename Func>
  struct delegate_t;

  template<typename R, typename... Args>
  struct delegate_t<R(Args...)> {
    using raw_func_ptr_t        = R (*)(void*, Args...);
    raw_func_ptr_t raw_func_ptr = nullptr;
    void* data_ptr              = nullptr;

    // Constructor from free function pointer.

    template<R (*)(Args...)>
    static delegate_t make();

    template<auto FuncPtr, typename Data>
      requires std::same_as<decltype(FuncPtr), R (*)(Data*, Args...)>
    static delegate_t make(Data*);

    // Constructors from member function pointer.

    template<auto MemberFuncPtr, typename T>
      requires std::same_as<decltype(MemberFuncPtr), R (T::*)(Args...)>
    static delegate_t make(T*);

    template<auto MemberFuncPtr, typename T>
      requires std::same_as<decltype(MemberFuncPtr), R (T::*)(Args...) const>
    static delegate_t make(const T*);

    bool has_value() const;

    void clear();

    R operator()(Args...) const;
  };
}

// IMPLEMENTATION

namespace silva {

  template<typename R, typename... Args>
  bool delegate_t<R(Args...)>::has_value() const
  {
    return raw_func_ptr != nullptr;
  }

  template<typename R, typename... Args>
  void delegate_t<R(Args...)>::clear()
  {
    raw_func_ptr = nullptr;
    data_ptr     = nullptr;
  }

  template<typename R, typename... Args>
  template<R (*FuncPtr)(Args...)>
  delegate_t<R(Args...)> delegate_t<R(Args...)>::make()
  {
    return {
        .raw_func_ptr = +[](void*, Args... args) -> R { return FuncPtr(std::move(args)...); },
        .data_ptr     = nullptr,
    };
  }

  template<typename R, typename... Args>
  template<auto FuncPtr, typename Data>
    requires std::same_as<decltype(FuncPtr), R (*)(Data*, Args...)>
  delegate_t<R(Args...)> delegate_t<R(Args...)>::make(Data* data_ptr)
  {
    return {
        .raw_func_ptr = +[](void* data_ptr, Args... args) -> R {
          return FuncPtr((Data*)(data_ptr), std::move(args)...);
        },
        .data_ptr = (void*)data_ptr,
    };
  }

  template<typename R, typename... Args>
  template<auto MemberFuncPtr, typename T>
    requires std::same_as<decltype(MemberFuncPtr), R (T::*)(Args...)>
  delegate_t<R(Args...)> delegate_t<R(Args...)>::make(T* ptr)
  {
    return {
        .raw_func_ptr = +[](void* data_ptr, Args... args) -> R {
          return (((T*)data_ptr)->*MemberFuncPtr)(std::move(args)...);
        },
        .data_ptr = (void*)ptr,
    };
  }

  template<typename R, typename... Args>
  template<auto MemberFuncPtr, typename T>
    requires std::same_as<decltype(MemberFuncPtr), R (T::*)(Args...) const>
  delegate_t<R(Args...)> delegate_t<R(Args...)>::make(const T* ptr)
  {
    return {
        .raw_func_ptr = +[](void* data_ptr, Args... args) -> R {
          return (((const T*)data_ptr)->*MemberFuncPtr)(std::move(args)...);
        },
        .data_ptr = (void*)ptr,
    };
  }

  template<typename R, typename... Args>
  R delegate_t<R(Args...)>::operator()(Args... args) const
  {
    return raw_func_ptr(data_ptr, std::move(args)...);
  }
}
