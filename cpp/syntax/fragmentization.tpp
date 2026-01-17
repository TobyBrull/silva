#include "fragmentization.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  using namespace Catch::Matchers;

  using enum codepoint_category_t;
  using enum fragment_category_t;
  using fragment_t = fragmentization_t::fragment_t;

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
      const array_t<fragment_t> expected_fragments{
          {WHITESPACE, {0, 0, 0}},
          {IDENTIFIER, {2, 0, 2}},
          {NEWLINE, {2, 9, 13}},
          {WHITESPACE, {3, 0, 14}},
      };
      CHECK(frag->fragments == expected_fragments);
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
      const array_t<fragment_t> expected_fragments{
          {WHITESPACE, {0, 0, 0}},   //
          {IDENTIFIER, {1, 0, 1}},   // def
          {NEWLINE, {1, 3, 4}},      //
          {INDENT, {2, 0, 5}},       //
          {IDENTIFIER, {2, 2, 7}},   // test
          {WHITESPACE, {2, 6, 11}},  //
          {OPERATOR, {2, 7, 12}},    // <
          {OPERATOR, {2, 8, 13}},    // >
          {WHITESPACE, {2, 9, 14}},  //
          {NUMBER, {2, 12, 17}},     // 0x0308
          {OPERATOR, {2, 18, 23}},   // ⊙
          {STRING, {2, 19, 26}},     // 'abc'
          {NEWLINE, {2, 24, 31}},    //
          {WHITESPACE, {3, 0, 32}},  //
          {STRING, {3, 2, 34}},      // "abc"
          {NEWLINE, {3, 7, 39}},     //
          {INDENT, {4, 0, 40}},      //
          {IDENTIFIER, {4, 4, 44}},  // deep
          {NEWLINE, {4, 8, 48}},     //
          {WHITESPACE, {5, 0, 49}},  //
          {COMMENT, {6, 2, 53}},     // # Comment
          {WHITESPACE, {6, 11, 62}}, //
          {DEDENT, {8, 0, 64}},      //
          {DEDENT, {8, 0, 64}},      //
          {IDENTIFIER, {8, 0, 64}},  // back
          {NEWLINE, {8, 4, 68}},     //
          {WHITESPACE, {9, 0, 69}},  //
      };
      CHECK(frag->fragments == expected_fragments);
    }
  }
}
