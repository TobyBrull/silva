#include "unicode.hpp"

#include <catch2/catch_all.hpp>

namespace silva::unicode::test {
  TEST_CASE("unicode", "[codepoint_t]")
  {
    using enum type_t;
    CHECK(codepoint_type(U' ') == SPACE);
    CHECK(codepoint_type(U'\n') == NEWLINE);
    CHECK(codepoint_type(U'0') == DIGIT);
    CHECK(codepoint_type(U'9') == DIGIT);
  }
}
