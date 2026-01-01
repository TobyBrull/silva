#pragma once

#include "expected.hpp"
#include "string.hpp"
#include "two_stage_table.hpp"

namespace silva::unicode {
  using codepoint_t = char32_t;

  enum class type_t {
    OTHER = 0,
    SPACE,
    NEWLINE,

    DIGIT,
    XID_START,
    XID_CONTINUE, // excluding DIGIT and XID_START

    PAREN_OPEN,
    PAREN_CLOSE,
    OPERATOR, // excluding PAREN_OPEN and PAREN_CLOSE
  };
  // Returns unexpected if the codepoint .
  expected_t<type_t> codepoint_type(codepoint_t);

  template<typename F>
    requires std::invocable<F, expected_t<void>, index_t, codepoint_t>
  expected_t<void> for_each_codepoint(string_view_t, F);

  template<typename T>
  using table_t = two_stage_table_t<codepoint_t, T, 8>;
}
