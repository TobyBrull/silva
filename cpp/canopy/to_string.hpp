#pragma once

#include "customization_point.hpp"
#include "enum.hpp"
#include "stream.hpp"
#include "string_or_view.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace silva {
  struct stream_out_t : public customization_point_t<void(stream_t*, const void*)> {
    template<typename T>
    constexpr void operator()(stream_t*, const T&) const;
  };
  inline constexpr stream_out_t stream_out;

  void stream_out_impl(stream_t*, const string_t&);
  void stream_out_impl(stream_t*, const string_view_t&);
  void stream_out_impl(stream_t*, const string_or_view_t&);

  template<typename T>
    requires std::is_arithmetic_v<T>
  void stream_out_impl(stream_t*, const T&);

  template<typename T, typename U>
  void stream_out_impl(stream_t*, const pair_t<T, U>&);

  template<typename... Ts>
  void stream_out_impl(stream_t*, const tuple_t<Ts...>&);

  template<typename... Ts>
  void stream_out_impl(stream_t*, const variant_t<Ts...>&);

  template<typename Enum>
    requires std::is_enum_v<Enum>
  void stream_out_impl(stream_t*, const Enum& x);

  struct to_string_value_t : public customization_point_t<string_t(const void*)> {
    template<typename T>
    constexpr string_t operator()(const T&) const;
  };
  inline constexpr to_string_value_t to_string_value;

#ifdef TRACY_ENABLE
#  define ZoneTextToString(x)              \
    {                                      \
      const auto sov = to_string_value(x); \
      const auto sv  = temp.content_str(); \
      ZoneText(sv.data(), sv.size());      \
    }
#endif
}

// IMPLEMENTATION

namespace silva {
  template<typename T>
  constexpr void stream_out_t::operator()(stream_t* stream, const T& x) const
  {
    using silva::stream_out_impl;
    return stream_out_impl(stream, x);
  }

  template<typename T>
  constexpr string_t to_string_value_t::operator()(const T& x) const
  {
    stream_memory_t temp;
    silva::stream_out(&temp, x);
    return temp.content_str_fetch();
  }

  inline void stream_out_impl(stream_t* stream, const string_t& x)
  {
    stream->write_str(x);
  }
  inline void stream_out_impl(stream_t* stream, const string_view_t& x)
  {
    stream->write_str(x);
  }
  inline void stream_out_impl(stream_t* stream, const string_or_view_t& x)
  {
    stream->write_str(x.as_string_view());
  }

  template<typename T>
    requires std::is_arithmetic_v<T>
  void stream_out_impl(stream_t* stream, const T& x)
  {
    stream->write_str(std::to_string(x));
  }

  template<typename T, typename U>
  void stream_out_impl(stream_t* stream, const pair_t<T, U>& x)
  {
    stream->format("[{} {}]", x.first, x.second);
  }

  template<typename... Ts>
  void stream_out_impl(stream_t* stream, const tuple_t<Ts...>& x)
  {
    stream->format("[{}]", fmt::join(x, " "));
  }

  template<typename... Ts>
  void stream_out_impl(stream_t* stream, const variant_t<Ts...>& x)
  {
    std::visit([stream](const auto& value) { stream_out(stream, value); }, x);
  }

  template<typename Enum>
    requires std::is_enum_v<Enum>
  void stream_out_impl(stream_t* stream, const Enum& x)
  {
    static const auto vals = enum_hashmap_to_string<Enum>();
    stream->write_str(string_view_t{vals.at(x)});
  }
}
