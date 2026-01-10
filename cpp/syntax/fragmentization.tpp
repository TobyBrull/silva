#include "fragmentization.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  using enum codepoint_category_t;
  using enum fragment_category_t;

  TEST_CASE("fragmentization-data", "[fragmentization_t]")
  {
    const unicode::table_t<codepoint_category_t>& cct = silva::codepoint_category_table;
    SILVA_EXPECT_REQUIRE(cct.validate());
    REQUIRE(cct.key_size() == 0x110000);

    CHECK(cct[U'\t'] == Forbidden);
    CHECK(cct[U'\n'] == Newline);
    CHECK(cct[U' '] == Space);
    CHECK(cct[U'*'] == Operator);
    CHECK(cct[U'⊙'] == Operator);
    CHECK(cct[U'«'] == ParenthesisLeft);
    CHECK(cct[U'»'] == ParenthesisRight);
    CHECK(cct[U'8'] == XID_Continue);
    CHECK(cct[U'A'] == XID_Start);
    CHECK(cct[U'_'] == XID_Start);
  }

  TEST_CASE("fragmentization", "[fragmentization_t]")
  {
    const auto frag = SILVA_EXPECT_REQUIRE(fragmentize("..", "\n\nx\n\n"));
    const array_t<fragment_category_t> expected_categories{
        WHITESPACE,
        IDENTIFIER,
        NEWLINE,
        WHITESPACE,
    };
    CHECK(frag->categories == expected_categories);
    const array_t<file_location_t> expected_locations{
        {0, 0, 0},
        {2, 0, 2},
        {2, 1, 3},
        {3, 0, 4},
    };
    CHECK(frag->locations == expected_locations);
  }
}
