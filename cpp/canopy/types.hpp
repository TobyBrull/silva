#pragma once

#include "config.hpp"
#include "tracy.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace silva {
  using index_t = int32_t;
  using byte_t  = std::byte;

  using string_t = std::string;
  void string_pad(string_t&, index_t size, char fill_char = ' ');

  using string_view_t = std::string_view;

  template<typename T, typename Deleter = std::default_delete<T>>
  using unique_ptr_t = std::unique_ptr<T, Deleter>;

  template<typename T>
  using shared_ptr_t = std::shared_ptr<T>;

  template<typename T>
  shared_ptr_t<T> share(unique_ptr_t<T> ptr)
  {
    return shared_ptr_t<T>(ptr.release());
  }

  template<typename T>
  using optional_t = std::optional<T>;

  struct none_t {
    operator std::nullopt_t() const { return std::nullopt; }
    friend auto operator<=>(const none_t&, const none_t&) = default;
  };
  constexpr none_t none = {};

  template<typename T, typename U>
  using pair_t = std::pair<T, U>;

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

  using filesystem_path_t = std::filesystem::path;

  template<typename T>
  using function_t = std::function<T>;

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
      this->pop_back();
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
