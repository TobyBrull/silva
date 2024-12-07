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

  struct sprite_t {
    sprite_t() = default;

    sprite_t(sprite_t&&)            = default;
    sprite_t& operator=(sprite_t&&) = default;

    // Sprites are encouraged to implement an explicity "copy" function (they don't have to), but
    // implicit copy is disabled.

    sprite_t(const sprite_t&)            = delete;
    sprite_t& operator=(const sprite_t&) = delete;
  };
}
