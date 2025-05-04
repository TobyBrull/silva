#pragma once

#include <fmt/format.h>

#include <source_location>

namespace silva {
  using source_location_t = std::source_location;

  constexpr static inline source_location_t source_location_none = {};
}

// IMPLEMENTATION

template<>
struct fmt::formatter<silva::source_location_t> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(const silva::source_location_t& x, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "[{}:{}:{}]", x.function_name(), x.line(), x.column());
  }
};
