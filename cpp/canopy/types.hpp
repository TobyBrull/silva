#pragma once

#include "config.hpp"
#include "tracy.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>

namespace silva {
  using index_t = int32_t;
  using byte_t  = std::byte;

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
