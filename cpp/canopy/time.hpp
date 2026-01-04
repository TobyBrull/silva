#pragma once

#include "expected.hpp"
#include "types.hpp"

#include <fmt/format.h>

namespace silva {
  using time_repr_t = int64_t;

  struct time_span_t {
    time_repr_t nanos = 0;

    time_span_t() = default;
    explicit time_span_t(time_repr_t nanos_since_epoch);

    friend auto operator<=>(const time_span_t&, const time_span_t&) = default;

    string_t to_string_default() const; // "365/13:03:44/123.456.789"

    static expected_t<time_span_t> from_string_default(string_view_t);

    friend std::ostream& operator<<(std::ostream&, const time_span_t&);
  };

  // Basically std::chrono::system_clock::time_point
  struct time_point_t {
    // Nanoseconds since 1970-01-01 00:00:00 UTC, not counting leap seconds (i.e., Unix Time)
    time_repr_t nanos_since_epoch = std::numeric_limits<time_repr_t>::min();

    time_point_t() = default;
    explicit time_point_t(time_repr_t nanos_since_epoch);

    bool is_none() const;

    static time_point_t now();

    friend auto operator<=>(const time_point_t&, const time_point_t&) = default;

    string_t to_string_default() const; // "2025-05-04/13:03:44/123.456.789"
    string_t to_string_ostream() const; // "2025-05-04 13:03:44.123456789"

    static expected_t<time_point_t> from_string_default(string_view_t);
    static expected_t<time_point_t> from_string_ostream(string_view_t);

    friend std::ostream& operator<<(std::ostream&, const time_point_t&);
  };

  time_point_t operator"" _time_point(const char* str, std::size_t);
  time_span_t operator"" _time_span(const char* str, std::size_t);

  constexpr static inline time_point_t time_point_none = {};

  time_span_t operator-(time_span_t, time_span_t);
  time_span_t operator+(time_span_t, time_span_t);
  time_span_t operator-(time_point_t, time_point_t);
  time_point_t operator+(time_point_t, time_span_t);
  time_point_t operator-(time_point_t, time_span_t);
}

// IMPLEMENTATION

template<>
struct fmt::formatter<silva::time_span_t> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(const silva::time_span_t& ts, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "{}", ts.to_string_default());
  }
};

template<>
struct fmt::formatter<silva::time_point_t> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(const silva::time_point_t& tp, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "{}", tp.to_string_default());
  }
};
