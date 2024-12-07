#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace silva {
  using index_t = uint32_t;

  using string_t = std::string;

  using string_view_t = std::string_view;

  template<typename T>
  using optional_t = std::optional<T>;

  constexpr std::nullopt_t none = std::nullopt;

  template<typename T>
  using variant_t = std::variant<T>;

  template<typename T>
  using span_t = std::span<T>;

  template<typename T>
  using vector_t = std::vector<T>;

  template<typename K, typename V>
  using hashmap_t = std::unordered_map<K, V>;

  using filesystem_path_t = std::filesystem::path;

  template<typename T>
  struct field_t {
    index_t offset = 0;
    index_t size   = 0;

    span_t<T> to_span(T* data) const;
    span_t<const T> to_span(const T* data) const;
  };

  struct sprite_t {
    sprite_t(sprite_t&&)            = default;
    sprite_t& operator=(sprite_t&&) = default;

    sprite_t(const sprite_t&)            = delete;
    sprite_t& operator=(const sprite_t&) = delete;
  };

  void string_append_escaped(std::string& output_buffer, string_view_t unescaped_string);
  void string_append_unescaped(std::string& output_buffer, string_view_t escaped_string);

  std::string string_escaped(string_view_t unescaped_string);
  std::string string_unescaped(string_view_t escaped_string);
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
