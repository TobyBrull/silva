#pragma once

#include "types.hpp"

namespace silva {
  struct string_or_view_t {
    variant_t<string_view_t, string_t> data;

    string_or_view_t() : data(string_view_t{}) {}
    explicit string_or_view_t(string_view_t x) : data(x) {}
    explicit string_or_view_t(string_t&& x) : data(std::move(x)) {}

    string_view_t get_view() const
    {
      return std::visit([](const auto& dd) -> string_view_t { return dd; }, data);
    }

    friend bool operator==(const string_or_view_t& lhs, const string_or_view_t& rhs)
    {
      return lhs.get_view() == rhs.get_view();
    }
    friend auto operator<=>(const string_or_view_t& lhs, const string_or_view_t& rhs)
    {
      return lhs.get_view() <=> rhs.get_view();
    }
  };
}

// IMPLEMENTATION

namespace std {
  template<>
  struct hash<silva::string_or_view_t> {
    std::size_t operator()(const silva::string_or_view_t& x) const
    {
      return std::hash<std::string_view>{}(x.get_view());
    }
  };
}
