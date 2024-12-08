#pragma once

#include "types.hpp"

namespace silva {

  template<typename T>
  struct field_t {
    index_t offset = 0;
    index_t size   = 0;

    span_t<T> to_span(T* data) const { return span_t<T>(data + offset, size); }
    span_t<const T> to_span(const T* data) const { return span_t<const T>(data + offset, size); }

    string_view_t to_string_view(const char* data) const
      requires std::same_as<T, char>
    {
      return string_view_t(data + offset, size);
    }

    friend auto operator<=>(const field_t&, const field_t&) = default;
  };

  template<typename T>
  struct field_hash_t;

  template<>
  struct field_hash_t<char> {
    using is_transparent = void;

    using hash_t = std::hash<string_view_t>;

    using Ptr        = const char*;
    mutable Ptr data = nullptr;

    std::size_t operator()(const field_t<char>& field) const
    {
      return hash_t{}(field.to_string_view(data));
    }
    std::size_t operator()(const string_view_t& sv) const { return hash_t{}(sv); }
  };

  template<typename T>
  struct field_pred_t {
    using is_transparent = void;

    using Ptr        = const T*;
    mutable Ptr data = nullptr;

    bool operator()(const field_t<T>& lhs, const field_t<T>& rhs) const
    {
      return std::ranges::equal(lhs.to_span(data), rhs.to_span(data));
    }
    bool operator()(const field_t<char>& lhs, const string_view_t& rhs) const
      requires std::same_as<T, char>
    {
      return lhs.to_string_view(data) == rhs;
    }
    bool operator()(const string_view_t& lhs, const field_t<char>& rhs) const
      requires std::same_as<T, char>
    {
      return lhs == rhs.to_string_view(data);
    }
    bool operator()(const string_view_t& lhs, const string_view_t& rhs) const
      requires std::same_as<T, char>
    {
      return lhs == rhs;
    }
  };

  template<typename T, typename V>
  struct field_hashmap_t {
    using used_hashmap_t = std::unordered_map<field_t<T>, V, field_hash_t<T>, field_pred_t<T>>;

    used_hashmap_t store;

    field_hashmap_t(const T* data) : store(0, field_hash_t<T>{data}, field_pred_t<T>{data}) {}

    field_hashmap_t(field_hashmap_t&&)            = delete;
    field_hashmap_t& operator=(field_hashmap_t&&) = delete;

    field_hashmap_t(const field_hashmap_t&)            = delete;
    field_hashmap_t& operator=(const field_hashmap_t&) = delete;

    field_hashmap_t move(const T* new_data);
    field_hashmap_t copy(const T* new_data);

    template<typename K>
    auto find(this auto& self, const K& key)
    {
      return self.store.find(key);
    }
    auto begin(this auto& self) { return self.store.begin(); }
    auto end(this auto& self) { return self.store.end(); }

    template<typename... Args>
    auto emplace(this auto& self, Args&&... args)
    {
      return self.store.emplace(std::forward<Args>(args)...);
    }
  };
}

// IMPLEMENTATION

namespace silva {
}
