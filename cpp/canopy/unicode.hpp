#pragma once

#include "expected.hpp"
#include "string.hpp"
#include "two_stage_table.hpp"

namespace silva::unicode {
  using codepoint_t = char32_t;

  void utf8_encode_one(string_t& output, codepoint_t);

  expected_t<tuple_t<codepoint_t, index_t>> utf8_decode_one(string_view_t);

  template<typename F>
  expected_t<void> for_each_codepoint(string_view_t, F);

  template<typename T>
  using table_t = two_stage_table_t<codepoint_t, T, 8>;
}

// IMPLEMENTATION

namespace silva::unicode {
  template<typename F>
  expected_t<void> for_each_codepoint(const string_view_t s, F f)
  {
    static_assert(std::invocable<F, codepoint_t, index_t, index_t>);
    index_t pos = 0;
    while (pos < s.size()) {
      const auto [codepoint, len] =
          SILVA_EXPECT_FWD(utf8_decode_one(s.substr(pos)), "unable to decode codepoint at {}", pos);
      SILVA_EXPECT_FWD(f(codepoint, pos, len));
      pos += len;
    }
    SILVA_EXPECT(pos == s.size(), MINOR, "not able to consume complete string");
    return {};
  }
}
