#pragma once

#include "byte_sink.hpp"
#include "customization_point.hpp"
#include "enum.hpp"
#include "string_or_view.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace silva {

  struct pretty_write_t : public customization_point_t<void(const void*, byte_sink_t*)> {
    template<typename T>
    constexpr void operator()(const T&, byte_sink_t*) const;
  };
  inline constexpr pretty_write_t pretty_write;

  void pretty_write_impl(const string_t&, byte_sink_t*);
  void pretty_write_impl(const string_view_t&, byte_sink_t*);
  void pretty_write_impl(const string_or_view_t&, byte_sink_t*);

  template<typename T>
    requires std::is_arithmetic_v<T>
  void pretty_write_impl(const T&, byte_sink_t*);

  template<typename T, typename U>
  void pretty_write_impl(const pair_t<T, U>&, byte_sink_t*);

  template<typename... Ts>
  void pretty_write_impl(const tuple_t<Ts...>&, byte_sink_t*);

  template<typename... Ts>
  void pretty_write_impl(const variant_t<Ts...>&, byte_sink_t*);

  template<typename T>
  void pretty_write_impl(const span_t<T>&, byte_sink_t*);

  template<typename T>
  void pretty_write_impl(const array_t<T>&, byte_sink_t*);

  template<typename T, size_t N>
  void pretty_write_impl(const array_fixed_t<T, N>&, byte_sink_t*);

  template<typename Enum>
    requires std::is_enum_v<Enum>
  void pretty_write_impl(const Enum& x, byte_sink_t*);

  struct pretty_string_t : public customization_point_t<string_t(const void*)> {
    template<typename T>
    constexpr string_t operator()(const T&) const;
  };
  inline constexpr pretty_string_t pretty_string;

#ifdef TRACY_ENABLE
#  define ZoneTextPrettyString(x)          \
    {                                      \
      const auto sov = pretty_string(x);   \
      const auto sv  = temp.content_str(); \
      ZoneText(sv.data(), sv.size());      \
    }
#endif
}

// CATCH2 GLUE CODE

namespace silva {
  template<typename T>
    requires(!std::same_as<T, string_t> && !std::same_as<T, string_view_t> &&
             !std::same_as<T, char> &&
             requires { pretty_write_impl(std::declval<T>(), std::declval<byte_sink_t*>()); })
  std::ostream& operator<<(std::ostream& os, const T& x)
  {
    byte_sink_memory_t temp;
    silva::pretty_write(x, &temp);
    return os << temp.content_str_fetch();
  }
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  constexpr void pretty_write_t::operator()(const T& x, byte_sink_t* byte_sink) const
  {
    using silva::pretty_write_impl;
    pretty_write_impl(x, byte_sink);
  }

  template<typename T>
  constexpr string_t pretty_string_t::operator()(const T& x) const
  {
    byte_sink_memory_t temp;
    silva::pretty_write(x, &temp);
    return temp.content_str_fetch();
  }

  inline void pretty_write_impl(const string_t& x, byte_sink_t* byte_sink)
  {
    byte_sink->write_str(x);
  }
  inline void pretty_write_impl(const string_view_t& x, byte_sink_t* byte_sink)
  {
    byte_sink->write_str(x);
  }
  inline void pretty_write_impl(const string_or_view_t& x, byte_sink_t* byte_sink)
  {
    byte_sink->write_str(x.as_string_view());
  }

  template<typename T>
    requires std::is_arithmetic_v<T>
  void pretty_write_impl(const T& x, byte_sink_t* byte_sink)
  {
    byte_sink->write_str(std::to_string(x));
  }

  template<typename T, typename U>
  void pretty_write_impl(const pair_t<T, U>& x, byte_sink_t* byte_sink)
  {
    byte_sink->write_str("[");
    silva::pretty_write(x.first, byte_sink);
    byte_sink->write_str(" ");
    silva::pretty_write(x.second, byte_sink);
    byte_sink->write_str("]");
  }

  template<typename... Ts>
  void pretty_write_impl(const tuple_t<Ts...>& x, byte_sink_t* byte_sink)
  {
    byte_sink->write_str("[ ");
    std::apply(
        [&](const auto&... xx) {
          ((silva::pretty_write(xx, byte_sink), byte_sink->write_str(" ")), ...);
        },
        x);

    byte_sink->write_str("]");
  }

  template<typename... Ts>
  void pretty_write_impl(const variant_t<Ts...>& x, byte_sink_t* byte_sink)
  {
    std::visit([byte_sink](const auto& value) { pretty_write(value, byte_sink); }, x);
  }

  template<typename T>
  void pretty_write_impl(const span_t<T>& x, byte_sink_t* byte_sink)
  {
    byte_sink->write_str("[ ");
    for (const auto& elem: x) {
      silva::pretty_write(elem, byte_sink);
      byte_sink->write_str(" ");
    }
    byte_sink->write_str("]");
  }

  template<typename T>
  void pretty_write_impl(const array_t<T>& x, byte_sink_t* byte_sink)
  {
    pretty_write_impl(span_t<const T>{x.data(), x.size()}, byte_sink);
  }

  template<typename T, size_t N>
  void pretty_write_impl(const array_fixed_t<T, N>& x, byte_sink_t* byte_sink)
  {
    pretty_write_impl(span_t<const T>{x}, byte_sink);
  }

  template<typename Enum>
    requires std::is_enum_v<Enum>
  void pretty_write_impl(const Enum& x, byte_sink_t* byte_sink)
  {
    static const auto vals = enum_hashmap_to_string<Enum>();
    byte_sink->write_str(string_view_t{vals.at(x)});
  }
}
