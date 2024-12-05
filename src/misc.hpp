#pragma once

#include <cstdint>
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

  void string_append_escaped(std::string& output_buffer, std::string_view unescaped_string);
  void string_append_unescaped(std::string& output_buffer, std::string_view escaped_string);

  std::string string_escaped(std::string_view unescaped_string);
  std::string string_unescaped(std::string_view escaped_string);
}
