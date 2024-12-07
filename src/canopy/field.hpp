#pragma once

#include "types.hpp"

namespace silva {

  template<typename T>
  struct field_t {
    index_t offset = 0;
    index_t size   = 0;

    span_t<T> to_span(T* data) const;
    span_t<const T> to_span(const T* data) const;

    friend auto operator<=>(const field_t&, const field_t&) = default;
  };

  template<typename T>
  struct field_hash_t {
    const T* data = nullptr;
    std::size_t operator()(const field_t<T>& field) const
    {
      return std::hash<std::span<T>>{}(field.to_span(data));
    }
  };

  template<typename T>
  struct field_pred_t {
    const T* data = nullptr;
    bool operator()(const field_t<T>& lhs, const field_t<T>& rhs) const
    {
      return lhs.to_span(data) == rhs.to_span(data);
    }
  };

  template<typename T, typename V>
  struct field_hashmap_t
    : public std::unordered_map<field_t<T>, V, field_hash_t<T>, field_pred_t<T>> {
    field_hashmap_t(const T* data)
      : std::unordered_map<field_t<T>, V, field_hash_t<T>, field_pred_t<T>>(
            0, field_hash_t<T>{data}, field_pred_t<T>{data})
    {
    }
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  span_t<T> field_t<T>::to_span(T* data) const
  {
    return span_t<T>(data + offset, size);
  }

  template<typename T>
  span_t<const T> field_t<T>::to_span(const T* data) const
  {
    return span_t<const T>(data + offset, size);
  }
}
