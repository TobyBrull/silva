#pragma once

#include "array.hpp"
#include "assert.hpp"

#include <memory>

namespace silva {
  //
  // TODO: Exception safety
  //
  template<typename T, index_t N>
  struct small_vector_t {
    alignas(T) char storage[sizeof(T) * N];
    index_t size = 0;

    ~small_vector_t();

    small_vector_t() = default;

    small_vector_t(std::initializer_list<index_t>);

    small_vector_t(small_vector_t&&);
    small_vector_t& operator=(small_vector_t&&);
    small_vector_t(const small_vector_t&);
    small_vector_t& operator=(const small_vector_t&);

    auto* begin(this auto&& self) { return &(std::forward<decltype(self)>(self)[0]); }
    auto* end(this auto&& self) { return &(std::forward<decltype(self)>(self)[self.size]); }

    auto* data(this auto& self);
    operator span_t<T>();

    template<typename... Args>
    void emplace_back(Args&&...);

    T& operator[](index_t);
    const T& operator[](index_t) const;

    auto& back(this auto&);
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename T, index_t N>
  small_vector_t<T, N>::~small_vector_t()
  {
    for (index_t i = 0; i < size; ++i) {
      std::destroy_at<T>(&(*this)[i]);
    }
  }

  template<typename T, index_t N>
  small_vector_t<T, N>::small_vector_t(std::initializer_list<index_t> il)
  {
    SILVA_ASSERT(il.size() <= N);
    for (const auto& x: il) {
      emplace_back(x);
    }
  }

  template<typename T, index_t N>
  small_vector_t<T, N>::small_vector_t(small_vector_t&& other) : size(other.size)
  {
    for (index_t i = 0; i < size; ++i) {
      std::construct_at<T>(&(*this)[i], std::move(other[i]));
      std::destroy_at<T>(&other[i]);
    }
    other.size = 0;
  }

  template<typename T, index_t N>
  small_vector_t<T, N>& small_vector_t<T, N>::operator=(small_vector_t&& other)
  {
    if (this != &other) {
      for (index_t i = 0; i < size; ++i) {
        std::destroy_at<T>(&(*this)[i]);
      }
      size = other.size;
      for (index_t i = 0; i < size; ++i) {
        std::construct_at<T>(&(*this)[i], std::move(other[i]));
        std::destroy_at<T>(&other[i]);
      }
      other.size = 0;
    }
    return *this;
  }

  template<typename T, index_t N>
  small_vector_t<T, N>::small_vector_t(const small_vector_t& other) : size(other.size)
  {
    for (index_t i = 0; i < size; ++i) {
      std::construct_at<T>(&(*this)[i], other[i]);
    }
  }

  template<typename T, index_t N>
  small_vector_t<T, N>& small_vector_t<T, N>::operator=(const small_vector_t& other)
  {
    if (this != &other) {
      for (index_t i = 0; i < size; ++i) {
        std::destroy_at<T>(&(*this)[i]);
      }
      size = other.size;
      for (index_t i = 0; i < size; ++i) {
        std::construct_at<T>(&(*this)[i], std::move(other[i]));
      }
    }
    return *this;
  }

  template<typename T, index_t N>
  auto* small_vector_t<T, N>::data(this auto& self)
  {
    return &self[0];
  }

  template<typename T, index_t N>
  small_vector_t<T, N>::operator span_t<T>()
  {
    return span_t<T>{data(), (size_t)size};
  }

  template<typename T, index_t N>
  template<typename... Args>
  void small_vector_t<T, N>::emplace_back(Args&&... args)
  {
    SILVA_ASSERT(size < N);
    std::construct_at<T>(&(*this)[size], std::forward<Args>(args)...);
    ++size;
  }

  template<typename T, index_t N>
  T& small_vector_t<T, N>::operator[](const index_t index)
  {
    return *reinterpret_cast<T*>(storage + index * sizeof(T));
  }
  template<typename T, index_t N>
  const T& small_vector_t<T, N>::operator[](const index_t index) const
  {
    return *reinterpret_cast<const T*>(storage + index * sizeof(T));
  }
  template<typename T, index_t N>
  auto& small_vector_t<T, N>::back(this auto& self)
  {
    SILVA_ASSERT(self.size > 0);
    return self[self.size - 1];
  }
}
