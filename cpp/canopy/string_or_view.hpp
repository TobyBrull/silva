#pragma once

#include "hash.hpp"

#include <fmt/format.h>

namespace silva {
  struct string_or_view_t {
    variant_t<string_view_t, string_t> data;

    string_or_view_t() : data(string_view_t{}) {}
    string_or_view_t(string_view_t x) : data(x) {}
    string_or_view_t(string_t&& x) : data(std::move(x)) {}

    string_view_t as_string_view() const;
    string_t as_string(this auto&& self);

    friend bool operator==(const string_or_view_t& lhs, const string_or_view_t& rhs);
    friend auto operator<=>(const string_or_view_t& lhs, const string_or_view_t& rhs);

    friend hash_value_t hash_impl(const string_or_view_t& x) { return hash(x.as_string_view()); }
  };

  inline string_or_view_t operator"" _sov(const char* str, size_t)
  {
    return string_or_view_t{string_view_t{str}};
  }
}

// IMPLEMENTATION

namespace silva {
  inline string_view_t string_or_view_t::as_string_view() const
  {
    return std::visit([](const auto& dd) -> string_view_t { return dd; }, data);
  }
  inline string_t string_or_view_t::as_string(this auto&& self)
  {
    return std::visit(
        [](auto&& dd) -> string_t { return string_t{std::forward<decltype(dd)>(dd)}; },
        std::forward<decltype(self)>(self).data);
  }

  inline bool operator==(const string_or_view_t& lhs, const string_or_view_t& rhs)
  {
    return lhs.as_string_view() == rhs.as_string_view();
  }
  inline auto operator<=>(const string_or_view_t& lhs, const string_or_view_t& rhs)
  {
    return lhs.as_string_view() <=> rhs.as_string_view();
  }
}

namespace std {
  template<>
  struct hash<silva::string_or_view_t> {
    std::size_t operator()(const silva::string_or_view_t& x) const
    {
      return std::hash<std::string_view>{}(x.as_string_view());
    }
  };
}

template<>
struct fmt::formatter<silva::string_or_view_t> : fmt::formatter<silva::string_view_t> {
  template<typename FormatContext>
  auto format(const silva::string_or_view_t& s, FormatContext& ctx) const
  {
    return fmt::formatter<std::string_view>::format(s.as_string_view(), ctx);
  }
};
