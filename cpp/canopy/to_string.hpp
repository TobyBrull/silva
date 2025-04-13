#pragma once

#include "customization_point.hpp"
#include "enum.hpp"
#include "string_or_view.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace silva {
  struct to_string_t : public customization_point_t<string_or_view_t(const void*)> {
    template<typename T>
    constexpr string_or_view_t operator()(const T&) const;
  };
  inline constexpr to_string_t to_string;

  string_or_view_t to_string_impl(const string_t&);
  string_or_view_t to_string_impl(const string_view_t&);
  string_or_view_t to_string_impl(const string_or_view_t&);

  template<typename T>
    requires std::is_arithmetic_v<T>
  string_or_view_t to_string_impl(const T&);

  template<typename T, typename U>
  string_or_view_t to_string_impl(const pair_t<T, U>&);

  template<typename... Ts>
  string_or_view_t to_string_impl(const tuple_t<Ts...>&);

  template<typename... Ts>
  string_or_view_t to_string_impl(const variant_t<Ts...>&);

  template<typename Enum>
    requires std::is_enum_v<Enum>
  string_or_view_t to_string_impl(const Enum& x);
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  constexpr string_or_view_t to_string_t::operator()(const T& x) const
  {
    using silva::to_string_impl;
    return to_string_impl(x);
  }

  inline string_or_view_t to_string_impl(const string_t& x)
  {
    return string_t{x};
  }
  inline string_or_view_t to_string_impl(const string_view_t& x)
  {
    return x;
  }
  inline string_or_view_t to_string_impl(const string_or_view_t& x)
  {
    return x;
  }

  template<typename T>
    requires std::is_arithmetic_v<T>
  string_or_view_t to_string_impl(const T& x)
  {
    return std::to_string(x);
  }

  template<typename T, typename U>
  string_or_view_t to_string_impl(const pair_t<T, U>& x)
  {
    return fmt::format("[{} {}]", x.first, x.second);
  }

  template<typename... Ts>
  string_or_view_t to_string_impl(const tuple_t<Ts...>& x)
  {
    return fmt::format("[{}]", fmt::join(x, " "));
  }

  template<typename... Ts>
  string_or_view_t to_string_impl(const variant_t<Ts...>& x)
  {
    return std::visit([](const auto& value) -> string_or_view_t { return to_string(value); }, x);
  }

  template<typename Enum>
    requires std::is_enum_v<Enum>
  string_or_view_t to_string_impl(const Enum& x)
  {
    static const auto vals = enum_hashmap_to_string<Enum>();
    return string_view_t{vals.at(x)};
  }
}
