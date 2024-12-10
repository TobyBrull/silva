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

  template<typename T, typename Deleter = std::default_delete<T>>
  using unique_ptr_t = std::unique_ptr<T, Deleter>;

  template<typename T>
  using optional_t = std::optional<T>;

  using nullopt_t               = std::nullopt_t;
  constexpr std::nullopt_t none = std::nullopt;

  template<typename... Ts>
  using variant_t = std::variant<Ts...>;

  template<typename T>
  using span_t = std::span<T>;

  template<typename T>
  using vector_t = std::vector<T>;

  template<typename T, index_t N>
  using array_t = std::array<T, N>;

  template<typename K, typename V>
  using hashmap_t = std::unordered_map<K, V>;

  using filesystem_path_t = std::filesystem::path;

  struct menhir_t {
    menhir_t() = default;

    menhir_t(menhir_t&&)            = delete;
    menhir_t& operator=(menhir_t&&) = delete;

    menhir_t(const menhir_t&)            = delete;
    menhir_t& operator=(const menhir_t&) = delete;

    // All members of a menhir_t should be "const".
  };

  struct sprite_t {
    sprite_t() = default;

    sprite_t(sprite_t&&)            = default;
    sprite_t& operator=(sprite_t&&) = default;

    // Sprites are encouraged to implement an explicity "copy" function (they don't have to), but
    // implicit copy is disabled.

    sprite_t(const sprite_t&)            = delete;
    sprite_t& operator=(const sprite_t&) = delete;
  };

  template<typename T>
    requires std::derived_from<T, sprite_t>
  unique_ptr_t<T> to_unique_ptr(T x)
  {
    return std::make_unique<T>(std::move(x));
  }
}
