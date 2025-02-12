#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace silva {
  using index_t = uint32_t;
  using byte_t  = std::byte;

  using string_t = std::string;

  using string_view_t = std::string_view;

  template<typename T, typename Deleter = std::default_delete<T>>
  using unique_ptr_t = std::unique_ptr<T, Deleter>;

  template<typename T>
  using optional_t = std::optional<T>;

  using none_t          = std::nullopt_t;
  constexpr none_t none = std::nullopt;

  template<typename... Ts>
  using variant_t = std::variant<Ts...>;

  template<typename... Ts>
  using tuple_t = std::tuple<Ts...>;

  template<typename T>
  using span_t = std::span<T>;

  template<typename T>
  struct vector_t : public std::vector<T> {
    using std::vector<T>::vector;
    vector_t()                           = default;
    vector_t(const vector_t&)            = default;
    vector_t& operator=(const vector_t&) = default;
    vector_t(vector_t&&)                 = default;
    vector_t& operator=(vector_t&&)      = default;
    ~vector_t();
  };

  template<typename T>
  span_t<T> optional_to_span(optional_t<T>& x);

  template<typename T, index_t N>
  using array_t = std::array<T, N>;

  template<typename K, typename V>
  using hashmap_t = std::unordered_map<K, V>;

  template<typename T>
  using hashset_t = std::unordered_set<T>;

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

  template<typename T>
  unique_ptr_t<T> to_unique_ptr(T x)
  {
    return std::make_unique<T>(std::move(x));
  }

  template<class Enum>
    requires std::is_enum_v<Enum>
  constexpr std::underlying_type_t<Enum> to_int(const Enum e) noexcept
  {
    return static_cast<std::underlying_type_t<Enum>>(e);
  }
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  vector_t<T>::~vector_t()
  {
    while (!this->empty()) {
      this->resize(this->size() - 1);
    }
  }

  template<typename T>
  span_t<T> optional_to_span(optional_t<T>& x)
  {
    if (x.has_value()) {
      return span_t<T>{&x.value(), 1};
    }
    else {
      return {};
    }
  }
}
