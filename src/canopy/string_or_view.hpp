#pragma once

#include "types.hpp"

namespace silva {
  struct string_or_view_t {
    variant_t<string_view_t, string_t> data;

    string_or_view_t(string_view_t x) : data(x) {}
    string_or_view_t(string_t&& x) : data(std::move(x)) {}

    string_view_t get_view() const
    {
      return std::visit([](const auto& dd) -> string_view_t { return dd; }, data);
    }
  };
}
