#pragma once

#include "expected.hpp"
#include "string.hpp"
#include "two_stage_table.hpp"

namespace silva::unicode {
  using codepoint_t = char32_t;

  template<typename F>
    requires std::invocable<F, expected_t<void>, index_t, codepoint_t>
  expected_t<void> for_each_codepoint(string_view_t, F);

  template<typename T>
  using table_t = two_stage_table_t<codepoint_t, T, 8>;
}
