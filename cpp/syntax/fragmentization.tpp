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
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", "zyẍ_\n"));
      CHECK_THAT(err_msg, ContainsSubstring("Forbidden codepoint"));
      CHECK_THAT(err_msg, ContainsSubstring("0x0308"));
    }
    SECTION("error")
    {
      {
        const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", ""));
        CHECK_THAT(err_msg, ContainsSubstring("source-code expected to end with newline"));
      }
      SILVA_EXPECT_REQUIRE(fragmentize("..", "\n"));
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
    SECTION("parentheses")
    {
      const auto text = R"(
def
    id # Ho
        id (
b    # Hi
  c
)
    id
)";
      const auto frag = SILVA_EXPECT_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {WHITESPACE, {0, 0, 0}},   //
          {IDENTIFIER, {1, 0, 1}},   // def
          {NEWLINE, {1, 3, 4}},      //
          {INDENT, {2, 0, 5}},       //
          {IDENTIFIER, {2, 4, 9}},   // id
          {WHITESPACE, {2, 6, 11}},  //
          {COMMENT, {2, 7, 12}},     //
          {NEWLINE, {2, 11, 16}},    //
          {INDENT, {3, 0, 17}},      //
          {IDENTIFIER, {3, 8, 25}},  // id
          {WHITESPACE, {3, 10, 27}}, //
          {PAREN_LEFT, {3, 11, 28}}, // (
          {WHITESPACE, {3, 12, 29}}, //
          {IDENTIFIER, {4, 0, 30}},  // b
          {WHITESPACE, {4, 1, 31}},  //
          {COMMENT, {4, 5, 35}},     // # Hi
          {WHITESPACE, {4, 9, 39}},  //
          {IDENTIFIER, {5, 2, 42}},  // c
          {WHITESPACE, {5, 3, 43}},  //
          {PAREN_RIGHT, {6, 0, 44}}, // )
          {NEWLINE, {6, 1, 45}},     //
          {DEDENT, {7, 0, 46}},      //
          {IDENTIFIER, {7, 4, 50}},  // id
          {NEWLINE, {7, 6, 52}},     //
          {DEDENT, {8, 0, 53}},      //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("line continuation")
    {
      const auto text = R"(
def # Hi \
  'abc#\
xyz'
  var⦚abc#
     ⦚xyz
  retval \
y
)";
      const auto frag = SILVA_EXPECT_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {WHITESPACE, {0, 0, 0}},  //
          {IDENTIFIER, {1, 0, 1}},  // def
          {WHITESPACE, {1, 3, 4}},  //
          {COMMENT, {1, 4, 5}},     // '# Hi \'
          {NEWLINE, {1, 10, 11}},   //
          {INDENT, {2, 0, 12}},     //
          {STRING, {2, 2, 14}},     // 'abc#xyz'
          {NEWLINE, {3, 5, 25}},    //
          {WHITESPACE, {4, 0, 26}}, //
          {IDENTIFIER, {4, 2, 28}}, // var
          {STRING, {4, 5, 31}},     // 'abc#\nxyz'
          {NEWLINE, {5, 9, 46}},    //
          {WHITESPACE, {6, 0, 47}}, //
          {IDENTIFIER, {6, 2, 49}}, // retval
          {WHITESPACE, {6, 8, 57}}, //
          {IDENTIFIER, {7, 0, 60}}, // y
          {NEWLINE, {7, 1, 61}},    //
      };
      // CHECK(frag->fragments == expected_fragments);
    }
    SECTION("language")
    {
      const auto text = R"(
var x = Python ⎢def f(x, y):
               ⎢ return (x +
               ⎢    y)
               ⎢
               ⎢x = C ⎢int main () {
               ⎢      ⎢  x = ⦚Hello
               ⎢      ⎢      ⦚World ⎢ 42
               ⎢      ⎢  return 42;
               ⎢      ⎢}

var x = Python «
def f(x, y):
  return (x +
y )

x = language C « int main () {
  return 42;
} »
»
)";
      const auto frag = SILVA_EXPECT_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {WHITESPACE, {0, 0, 0}}, //
          {WHITESPACE, {0, 0, 0}}, //
      };
      // CHECK(frag->fragments == expected_fragments);
    }
  }
}
