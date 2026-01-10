#pragma once

#include "expected.hpp"
#include "string.hpp"
#include "two_stage_table.hpp"

#include <generator>

namespace silva::unicode {
  using codepoint_t = char32_t;

  void utf8_encode_one(string_t& output, codepoint_t);
  string_t utf8_encode_one(codepoint_t);

  expected_t<tuple_t<codepoint_t, index_t>> utf8_decode_one(string_view_t);

  struct codepoint_data_t {
    codepoint_t codepoint = 0;
    index_t byte_offset   = 0;
    index_t len           = 0;

    friend auto operator<=>(const codepoint_data_t&, const codepoint_data_t&) = default;
  };

  std::generator<expected_t<codepoint_data_t>> utf8_decode_generator(string_view_t);

  template<typename F>
  expected_t<void> utf8_decode_for_each(string_view_t, F);

  template<typename T>
  using table_t = two_stage_table_t<codepoint_t, T, 8>;
}

// IMPLEMENTATION

namespace silva::unicode {
  template<typename F>
  expected_t<void> utf8_decode_for_each(const string_view_t s, F f)
  {
    static_assert(std::invocable<F, codepoint_data_t>);
    for (auto x: utf8_decode_generator(s)) {
      const codepoint_data_t cd = SILVA_EXPECT_FWD_PLAIN(std::move(x));
      SILVA_EXPECT_FWD_PLAIN(f(cd));
    }
    return {};
  }
}
