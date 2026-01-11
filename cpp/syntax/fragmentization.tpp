#include "fragmentization.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  using namespace Catch::Matchers;

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
    SECTION("forbidden codepoint")
    {
      const auto res = fragmentize("..", "zyẍ_\n");
      REQUIRE(!res.has_value());
      const auto err_msg = res.error().to_string_plain().as_string();
      CHECK_THAT(err_msg, ContainsSubstring("Forbidden codepoint"));
      CHECK_THAT(err_msg, ContainsSubstring("0x0308"));
    }
    SECTION("basic")
    {
      const auto text = R"(

xyz123_äß

)";
      const auto frag = SILVA_EXPECT_REQUIRE(fragmentize("..", text));
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
          {2, 9, 13},
          {3, 0, 14},
      };
      CHECK(frag->locations == expected_locations);
    }
    SECTION("ident")
    {
      const auto text = R"(
def
  test <>   0x0308⊙'abc'
  "abc"
    deep
 
  # Comment

back

)";
      const auto frag = SILVA_EXPECT_REQUIRE(fragmentize("..", text));
      const array_t<fragment_category_t> expected_categories{
          WHITESPACE, IDENTIFIER, NEWLINE, INDENT,     IDENTIFIER, WHITESPACE, OPERATOR,
          OPERATOR,   WHITESPACE, NUMBER,  OPERATOR,   STRING,     NEWLINE,    WHITESPACE,
          STRING,     NEWLINE,    INDENT,  IDENTIFIER, NEWLINE,    WHITESPACE, COMMENT,
          WHITESPACE, DEDENT,     DEDENT,  IDENTIFIER, NEWLINE,    WHITESPACE,
      };
      CHECK(frag->categories == expected_categories);
      const array_t<file_location_t> expected_locations{
          {0, 0, 0},   {1, 0, 1},  {1, 3, 4},   {2, 0, 5},   {2, 2, 7},   {2, 6, 11},  {2, 7, 12},
          {2, 8, 13},  {2, 9, 14}, {2, 12, 17}, {2, 18, 23}, {2, 19, 26}, {2, 24, 31}, {3, 0, 32},
          {3, 2, 34},  {3, 7, 39}, {4, 0, 40},  {4, 4, 44},  {4, 8, 48},  {5, 0, 49},  {6, 2, 53},
          {6, 11, 62}, {8, 0, 64}, {8, 0, 64},  {8, 0, 64},  {8, 4, 68},  {9, 0, 69},
      };
      CHECK(frag->locations == expected_locations);
    }
  }
}
