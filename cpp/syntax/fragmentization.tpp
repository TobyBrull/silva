#include "fragmentization.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  using namespace Catch::Matchers;

  using enum codepoint_category_t;
  using enum fragment_category_t;

  array_t<fragment_category_t> only_real_categories(const array_t<fragment_t>& x)
  {
    array_t<fragment_category_t> retval;
    retval.reserve(x.size());
    for (const auto& elem: x) {
      if (is_fragment_category_real(elem.category)) {
        retval.push_back(elem.category);
      }
    }
    return retval;
  }

  TEST_CASE("fragmentization-data", "[fragmentization_t]")
  {
    const unicode::table_t<codepoint_category_t>& cct = silva::codepoint_category_table;
    SILVA_REQUIRE(cct.validate());
    REQUIRE(cct.key_size() == 0x110000);

    CHECK(cct[U'\t'] == Forbidden);
    CHECK(cct[U'\n'] == Newline);
    CHECK(cct[U' '] == Space);
    CHECK(cct[U'*'] == Operator);
    CHECK(cct[U'⊙'] == Operator);
    CHECK(cct[U'«'] == ParenthesisLeft);
    CHECK(cct[U'»'] == ParenthesisRight);
    CHECK(cct[U'8'] == XID_Continue);
    CHECK(cct[U'A'] == XID_Uppercase);
    CHECK(cct[U'a'] == XID_Lowercase);
    CHECK(cct[U'_'] == XID_Start);
    CHECK(cct[U'林'] == XID_Start);
    CHECK(cct[U'-'] == Operator);
  }

  TEST_CASE("fragmentization", "[fragmentization_t]")
  {
    SECTION("error: forbidden codepoint")
    {
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize_unique("..", "zyẍ_\n"));
      CHECK_THAT(err_msg, ContainsSubstring("Forbidden codepoint"));
      CHECK_THAT(err_msg, ContainsSubstring("0x0308"));
    }
    SECTION("error: stray LANG_END")
    {
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize_unique("..", "»\n"));
      CHECK_THAT(err_msg, ContainsSubstring("unexpected '»'"));
    }
    SECTION("error: non-matching parentheses")
    {
      const auto text    = R"(
A ⎢ ( B
)
)";
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize_unique("..", text));
      CHECK_THAT(err_msg, ContainsSubstring("Expected multi-line language to continue"));
    }
    SECTION("empty")
    {
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize_unique("..", ""));
      CHECK_THAT(err_msg, ContainsSubstring("source-code expected to end with newline"));
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", "\n"));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},
          {WHITESPACE, {0, 0, 0}},
          {LANG_END, {0, 0, 0}},
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("error: newline in string")
    {
      const auto text    = R"('abc#\
xyz'
)";
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize_unique("..", text));
      CHECK_THAT(err_msg, ContainsSubstring("unexpected escape sequence"));
    }
    SECTION("basic")
    {
      const auto text = R"(

xyz123_äß

)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},
          {WHITESPACE, {0, 0, 0}},
          {WHITESPACE, {1, 0, 1}},
          {ID_LOWER, {2, 0, 2}},
          {ID_LOWER, {2, 1, 3}},
          {ID_LOWER, {2, 2, 4}},
          {DIGIT, {2, 3, 5}},
          {DIGIT, {2, 4, 6}},
          {DIGIT, {2, 5, 7}},
          {ID_START__NOT_ID_LOWER_AND_NOT_ID_UPPER, {2, 6, 8}},
          {ID_LOWER, {2, 7, 9}},
          {ID_LOWER, {2, 8, 11}},
          {NEWLINE, {2, 9, 13}},
          {WHITESPACE, {3, 0, 14}},
          {LANG_END, {3, 0, 14}},
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("kebab")
    {
      const auto text = "he-wo -++- he/wo\n";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}}, //
          {ID_LOWER, {0, 0, 0}},   //
          {ID_LOWER, {0, 1, 1}},   //
          {OPERATOR, {0, 2, 2}},   //
          {ID_LOWER, {0, 3, 3}},   //
          {ID_LOWER, {0, 4, 4}},   //
          {SPACE, {0, 5, 5}},      //
          {OPERATOR, {0, 6, 6}},   //
          {OPERATOR, {0, 7, 7}},   //
          {OPERATOR, {0, 8, 8}},   //
          {OPERATOR, {0, 9, 9}},   //
          {SPACE, {0, 10, 10}},    //
          {ID_LOWER, {0, 11, 11}}, //
          {ID_LOWER, {0, 12, 12}}, //
          {OPERATOR, {0, 13, 13}}, //
          {ID_LOWER, {0, 14, 14}}, //
          {ID_LOWER, {0, 15, 15}}, //
          {NEWLINE, {0, 16, 16}},  //
          {LANG_END, {0, 16, 16}},
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("indent")
    {
      const auto text = R"(
def
  test <>   0x0308⊙'abc'
  "abc"
    deep
 
  # Comment

back

)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},   //
          {WHITESPACE, {0, 0, 0}},   //
          {ID_LOWER, {1, 0, 1}},     // d
          {ID_LOWER, {1, 1, 2}},     // e
          {ID_LOWER, {1, 2, 3}},     // f
          {NEWLINE, {1, 3, 4}},      //
          {INDENT, {2, 0, 5}},       //
          {ID_LOWER, {2, 2, 7}},     // t
          {ID_LOWER, {2, 3, 8}},     // e
          {ID_LOWER, {2, 4, 9}},     // s
          {ID_LOWER, {2, 5, 10}},    // t
          {SPACE, {2, 6, 11}},       //
          {OPERATOR, {2, 7, 12}},    // <
          {OPERATOR, {2, 8, 13}},    // >
          {SPACE, {2, 9, 14}},       //
          {SPACE, {2, 10, 15}},      //
          {SPACE, {2, 11, 16}},      //
          {DIGIT, {2, 12, 17}},      // 0
          {ID_LOWER, {2, 13, 18}},   // x
          {DIGIT, {2, 14, 19}},      // 0
          {DIGIT, {2, 15, 20}},      // 3
          {DIGIT, {2, 16, 21}},      // 0
          {DIGIT, {2, 17, 22}},      // 8
          {OPERATOR, {2, 18, 23}},   // ⊙
          {STRING, {2, 19, 26}},     // 'abc'
          {NEWLINE, {2, 24, 31}},    //
          {STRING, {3, 2, 34}},      // "abc"
          {NEWLINE, {3, 7, 39}},     //
          {INDENT, {4, 0, 40}},      //
          {ID_LOWER, {4, 4, 44}},    // d
          {ID_LOWER, {4, 5, 45}},    // e
          {ID_LOWER, {4, 6, 46}},    // e
          {ID_LOWER, {4, 7, 47}},    // p
          {NEWLINE, {4, 8, 48}},     //
          {WHITESPACE, {5, 1, 50}},  //
          {COMMENT, {6, 2, 53}},     // # Comment
          {WHITESPACE, {6, 11, 62}}, //
          {WHITESPACE, {7, 0, 63}},  //
          {DEDENT, {8, 0, 64}},      //
          {DEDENT, {8, 0, 64}},      //
          {ID_LOWER, {8, 0, 64}},    // b
          {ID_LOWER, {8, 1, 65}},    // a
          {ID_LOWER, {8, 2, 66}},    // c
          {ID_LOWER, {8, 3, 67}},    // k
          {NEWLINE, {8, 4, 68}},     //
          {WHITESPACE, {9, 0, 69}},  //
          {LANG_END, {9, 0, 69}},    //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("indent-start")
    {
      const auto text = "  def\n";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}}, //
          {INDENT, {0, 0, 0}},     //
          {ID_LOWER, {0, 2, 2}},   // d
          {ID_LOWER, {0, 3, 3}},   // e
          {ID_LOWER, {0, 4, 4}},   // f
          {NEWLINE, {0, 5, 5}},    //
          {DEDENT, {0, 5, 5}},     //
          {LANG_END, {0, 5, 5}},   //
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
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},    //
          {WHITESPACE, {0, 0, 0}},    //
          {ID_LOWER, {1, 0, 1}},      // d
          {ID_LOWER, {1, 1, 2}},      // e
          {ID_LOWER, {1, 2, 3}},      // f
          {NEWLINE, {1, 3, 4}},       //
          {INDENT, {2, 0, 5}},        //
          {ID_LOWER, {2, 4, 9}},      // i
          {ID_LOWER, {2, 5, 10}},     // d
          {SPACE, {2, 6, 11}},        //
          {COMMENT, {2, 7, 12}},      //
          {NEWLINE, {2, 11, 16}},     //
          {INDENT, {3, 0, 17}},       //
          {ID_LOWER, {3, 8, 25}},     // i
          {ID_LOWER, {3, 9, 26}},     // d
          {SPACE, {3, 10, 27}},       //
          {PARENTHESIS, {3, 11, 28}}, // (
          {LINEFEED, {3, 12, 29}},    //
          {ID_LOWER, {4, 0, 30}},     // b
          {SPACE, {4, 1, 31}},        //
          {SPACE, {4, 2, 32}},        //
          {SPACE, {4, 3, 33}},        //
          {SPACE, {4, 4, 34}},        //
          {COMMENT, {4, 5, 35}},      // # Hi
          {LINEFEED, {4, 9, 39}},     //
          {SPACE, {5, 0, 40}},        //
          {SPACE, {5, 1, 41}},        //
          {ID_LOWER, {5, 2, 42}},     // c
          {LINEFEED, {5, 3, 43}},     //
          {PARENTHESIS, {6, 0, 44}},  // )
          {NEWLINE, {6, 1, 45}},      //
          {DEDENT, {7, 0, 46}},       //
          {ID_LOWER, {7, 4, 50}},     // i
          {ID_LOWER, {7, 5, 51}},     // d
          {NEWLINE, {7, 6, 52}},      //
          {DEDENT, {7, 6, 52}},       //
          {LANG_END, {7, 6, 52}},     //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("line-continuation")
    {
      const auto text = R"(
def # Hi \
  'ab\'c#xyz'
  var¶abc#
     ¶xy¶z
  retval \
y
  retval\
y
)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},  //
          {WHITESPACE, {0, 0, 0}},  //
          {ID_LOWER, {1, 0, 1}},    // d
          {ID_LOWER, {1, 1, 2}},    // e
          {ID_LOWER, {1, 2, 3}},    // f
          {SPACE, {1, 3, 4}},       //
          {COMMENT, {1, 4, 5}},     // # Hi
          {NEWLINE, {1, 10, 11}},   //
          {INDENT, {2, 0, 12}},     //
          {STRING, {2, 2, 14}},     // 'ab\'c#xyz'
          {NEWLINE, {2, 13, 25}},   //
          {ID_LOWER, {3, 2, 28}},   // v
          {ID_LOWER, {3, 3, 29}},   // a
          {ID_LOWER, {3, 4, 30}},   // r
          {STRING, {3, 5, 31}},     // abc"xy¶z
          {NEWLINE, {4, 10, 50}},   //
          {ID_LOWER, {5, 2, 53}},   // r
          {ID_LOWER, {5, 3, 54}},   // e
          {ID_LOWER, {5, 4, 55}},   // t
          {ID_LOWER, {5, 5, 56}},   // v
          {ID_LOWER, {5, 6, 57}},   // a
          {ID_LOWER, {5, 7, 58}},   // l
          {SPACE, {5, 8, 59}},      //
          {WHITESPACE, {5, 9, 60}}, //
          {ID_LOWER, {6, 0, 62}},   // y
          {NEWLINE, {6, 1, 63}},    //
          {ID_LOWER, {7, 2, 66}},   // r
          {ID_LOWER, {7, 3, 67}},   // e
          {ID_LOWER, {7, 4, 68}},   // t
          {ID_LOWER, {7, 5, 69}},   // v
          {ID_LOWER, {7, 6, 70}},   // a
          {ID_LOWER, {7, 7, 71}},   // l
          {WHITESPACE, {7, 8, 72}}, //
          {ID_LOWER, {8, 0, 74}},   // y
          {NEWLINE, {8, 1, 75}},    //
          {DEDENT, {8, 1, 75}},     //
          {LANG_END, {8, 1, 75}},   //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("two multi-line strings")
    {
      const auto text = R"(
x¶ abc
y¶ xyz
)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}}, //
          {WHITESPACE, {0, 0, 0}}, //
          {ID_LOWER, {1, 0, 1}},   // x
          {STRING, {1, 1, 2}},     // ' abc'
          {NEWLINE, {1, 6, 8}},    //
          {ID_LOWER, {2, 0, 9}},   // y
          {STRING, {2, 1, 10}},    // ' xyz'
          {NEWLINE, {2, 6, 16}},   //
          {LANG_END, {2, 6, 16}},  //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("language")
    {
      const auto text = R"(
Python ⎢def
       ⎢  return (x +
       ⎢ y)

Python «
def
  return (
x)
»
)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},    //
          {WHITESPACE, {0, 0, 0}},    //
          {ID_UPPER, {1, 0, 1}},      // P
          {ID_LOWER, {1, 1, 2}},      // y
          {ID_LOWER, {1, 2, 3}},      // t
          {ID_LOWER, {1, 3, 4}},      // h
          {ID_LOWER, {1, 4, 5}},      // o
          {ID_LOWER, {1, 5, 6}},      // n
          {SPACE, {1, 6, 7}},         //
          {LANG_BEGIN, {1, 7, 8}},    //
          {ID_LOWER, {1, 8, 11}},     // d
          {ID_LOWER, {1, 9, 12}},     // e
          {ID_LOWER, {1, 10, 13}},    // f
          {NEWLINE, {1, 11, 14}},     //
          {INDENT, {2, 8, 25}},       //
          {ID_LOWER, {2, 10, 27}},    // r
          {ID_LOWER, {2, 11, 28}},    // e
          {ID_LOWER, {2, 12, 29}},    // t
          {ID_LOWER, {2, 13, 30}},    // u
          {ID_LOWER, {2, 14, 31}},    // r
          {ID_LOWER, {2, 15, 32}},    // n
          {SPACE, {2, 16, 33}},       //
          {PARENTHESIS, {2, 17, 34}}, // (
          {ID_LOWER, {2, 18, 35}},    // x
          {SPACE, {2, 19, 36}},       //
          {OPERATOR, {2, 20, 37}},    // +
          {LINEFEED, {2, 21, 38}},    //
          {SPACE, {3, 8, 49}},        //
          {ID_LOWER, {3, 9, 50}},     // y
          {PARENTHESIS, {3, 10, 51}}, // )
          {NEWLINE, {3, 11, 52}},     //
          {DEDENT, {4, 0, 53}},       //
          {LANG_END, {4, 0, 53}},     //
          {NEWLINE, {4, 0, 53}},      //
          {WHITESPACE, {4, 0, 53}},   //
          {ID_UPPER, {5, 0, 54}},     // P
          {ID_LOWER, {5, 1, 55}},     // y
          {ID_LOWER, {5, 2, 56}},     // t
          {ID_LOWER, {5, 3, 57}},     // h
          {ID_LOWER, {5, 4, 58}},     // o
          {ID_LOWER, {5, 5, 59}},     // n
          {SPACE, {5, 6, 60}},        //
          {LANG_BEGIN, {5, 7, 61}},   //
          {WHITESPACE, {5, 8, 63}},   //
          {ID_LOWER, {6, 0, 64}},     // d
          {ID_LOWER, {6, 1, 65}},     // e
          {ID_LOWER, {6, 2, 66}},     // f
          {NEWLINE, {6, 3, 67}},      //
          {INDENT, {7, 0, 68}},       //
          {ID_LOWER, {7, 2, 70}},     // r
          {ID_LOWER, {7, 3, 71}},     // e
          {ID_LOWER, {7, 4, 72}},     // t
          {ID_LOWER, {7, 5, 73}},     // u
          {ID_LOWER, {7, 6, 74}},     // r
          {ID_LOWER, {7, 7, 75}},     // n
          {SPACE, {7, 8, 76}},        //
          {PARENTHESIS, {7, 9, 77}},  // (
          {LINEFEED, {7, 10, 78}},    //
          {ID_LOWER, {8, 0, 79}},     // x
          {PARENTHESIS, {8, 1, 80}},  // )
          {NEWLINE, {8, 2, 81}},      //
          {WHITESPACE, {9, 0, 82}},   //
          {DEDENT, {9, 0, 82}},       //
          {LANG_END, {9, 0, 82}},     //
          {NEWLINE, {9, 1, 84}},      //
          {LANG_END, {9, 1, 84}},     //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("language-parens")
    {
      const auto text = R"(
Py ⎢ (x +
   ⎢ ⎢ y
   ⎢ )
)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},   //
          {WHITESPACE, {0, 0, 0}},   //
          {ID_UPPER, {1, 0, 1}},     // P
          {ID_LOWER, {1, 1, 2}},     // y
          {SPACE, {1, 2, 3}},        //
          {LANG_BEGIN, {1, 3, 4}},   //
          {INDENT, {1, 4, 7}},       //
          {PARENTHESIS, {1, 5, 8}},  // (
          {ID_LOWER, {1, 6, 9}},     // x
          {SPACE, {1, 7, 10}},       //
          {OPERATOR, {1, 8, 11}},    // +
          {LINEFEED, {1, 9, 12}},    //
          {SPACE, {2, 4, 19}},       //
          {LANG_BEGIN, {2, 5, 20}},  //
          {INDENT, {2, 6, 23}},      //
          {ID_LOWER, {2, 7, 24}},    // y
          {NEWLINE, {2, 8, 25}},     //
          {DEDENT, {3, 5, 33}},      //
          {LANG_END, {3, 5, 33}},    //
          {NEWLINE, {3, 5, 33}},     //
          {PARENTHESIS, {3, 5, 33}}, //
          {NEWLINE, {3, 6, 34}},     //
          {DEDENT, {3, 6, 34}},      //
          {LANG_END, {3, 6, 34}},    //
          {NEWLINE, {3, 6, 34}},     //
          {WHITESPACE, {3, 6, 34}},  //
          {LANG_END, {3, 6, 34}},    //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("nested-language-1")
    {
      const auto text = R"(
Python ⎢def
       ⎢
       ⎢  ⎢ str ¶Hello
       ⎢  ⎢     ¶World ⎢ 42 ¶ zig
       ⎢
       ⎢  ⎢int «
       ⎢  ⎢    x »
)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},    //
          {WHITESPACE, {0, 0, 0}},    //
          {ID_UPPER, {1, 0, 1}},      // P
          {ID_LOWER, {1, 1, 2}},      // y
          {ID_LOWER, {1, 2, 3}},      // t
          {ID_LOWER, {1, 3, 4}},      // h
          {ID_LOWER, {1, 4, 5}},      // o
          {ID_LOWER, {1, 5, 6}},      // n
          {SPACE, {1, 6, 7}},         //
          {LANG_BEGIN, {1, 7, 8}},    //
          {ID_LOWER, {1, 8, 11}},     // d
          {ID_LOWER, {1, 9, 12}},     // e
          {ID_LOWER, {1, 10, 13}},    // f
          {NEWLINE, {1, 11, 14}},     //
          {WHITESPACE, {2, 8, 25}},   //
          {INDENT, {3, 8, 36}},       //
          {LANG_BEGIN, {3, 10, 38}},  //
          {INDENT, {3, 11, 41}},      //
          {ID_LOWER, {3, 12, 42}},    // s
          {ID_LOWER, {3, 13, 43}},    // t
          {ID_LOWER, {3, 14, 44}},    // r
          {SPACE, {3, 15, 45}},       //
          {STRING, {3, 16, 46}},      //
          {NEWLINE, {4, 33, 95}},     //
          {DEDENT, {5, 8, 106}},      //
          {LANG_END, {5, 8, 106}},    //
          {NEWLINE, {5, 8, 106}},     //
          {WHITESPACE, {5, 8, 106}},  //
          {LANG_BEGIN, {6, 10, 119}}, //
          {ID_LOWER, {6, 11, 122}},   // i
          {ID_LOWER, {6, 12, 123}},   // n
          {ID_LOWER, {6, 13, 124}},   // t
          {SPACE, {6, 14, 125}},      //
          {LANG_BEGIN, {6, 15, 126}}, //
          {WHITESPACE, {6, 16, 128}}, //
          {INDENT, {7, 11, 144}},     //
          {ID_LOWER, {7, 15, 148}},   // x
          {SPACE, {7, 16, 149}},      //
          {NEWLINE, {7, 17, 150}},    //
          {DEDENT, {7, 17, 150}},     //
          {LANG_END, {7, 17, 150}},   //
          {NEWLINE, {7, 18, 152}},    //
          {LANG_END, {7, 18, 152}},   //
          {NEWLINE, {7, 18, 152}},    //
          {WHITESPACE, {7, 18, 152}}, //
          {DEDENT, {7, 18, 152}},     //
          {LANG_END, {7, 18, 152}},   //
          {NEWLINE, {7, 18, 152}},    //
          {WHITESPACE, {7, 18, 152}}, //
          {LANG_END, {7, 18, 152}},   //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("nested-language-2")
    {
      const auto text     = R"(
A ⎢ B «
  ⎢  C ⎢ D
  ⎢    ⎢  E ¶ abc
  ⎢    ⎢    ¶ xyz
  ⎢    ⎢
  ⎢  F » 
)";
      const auto frag     = SILVA_REQUIRE(fragmentize_unique("..", text));
      const auto frag_cat = only_real_categories(frag->fragments);
      const array_t<fragment_category_t> expected_fragment_categories{
          LANG_BEGIN, //
          ID_UPPER,   // A
          SPACE,      //
          LANG_BEGIN, //
          INDENT,     //
          ID_UPPER,   // B
          SPACE,      //
          LANG_BEGIN, //
          INDENT,     //
          ID_UPPER,   // C
          SPACE,      //
          LANG_BEGIN, //
          INDENT,     //
          ID_UPPER,   // D
          NEWLINE,    //
          INDENT,     //
          ID_UPPER,   // E
          SPACE,      //
          STRING,     //
          NEWLINE,    //
          DEDENT,     //
          DEDENT,     //
          LANG_END,   //
          NEWLINE,    //
          ID_UPPER,   // F
          SPACE,      //
          NEWLINE,    //
          DEDENT,     //
          LANG_END,   //
          SPACE,      //
          NEWLINE,    //
          DEDENT,     //
          LANG_END,   //
          NEWLINE,    //
          LANG_END,   //
      };
      CHECK(frag_cat == expected_fragment_categories);
    }
    SECTION("nested-language-3")
    {
      const auto text     = R"(
A ⎢ B « C » D
)";
      const auto frag     = SILVA_REQUIRE(fragmentize_unique("..", text));
      const auto frag_cat = only_real_categories(frag->fragments);
      const array_t<fragment_category_t> expected_fragment_categories{
          LANG_BEGIN, //
          ID_UPPER,   // A
          SPACE,      //
          LANG_BEGIN, //
          INDENT,     //
          ID_UPPER,   // B
          SPACE,      //
          LANG_BEGIN, //
          INDENT,     //
          ID_UPPER,   // C
          SPACE,      //
          NEWLINE,    //
          DEDENT,     //
          LANG_END,   //
          SPACE,      //
          ID_UPPER,   // D
          NEWLINE,    //
          DEDENT,     //
          LANG_END,   //
          NEWLINE,    //
          LANG_END,   //
      };
      CHECK(frag_cat == expected_fragment_categories);
    }
    SECTION("simple-indent")
    {
      const auto text     = R"(
def
  abc

def
  xyz
)";
      const auto frag     = SILVA_REQUIRE(fragmentize_unique("..", text));
      const auto frag_cat = only_real_categories(frag->fragments);
      const array_t<fragment_category_t> expected_fragment_categories{
          LANG_BEGIN, //
          ID_LOWER,   // d
          ID_LOWER,   // e
          ID_LOWER,   // f
          NEWLINE,    //
          INDENT,     //
          ID_LOWER,   // a
          ID_LOWER,   // b
          ID_LOWER,   // c
          NEWLINE,    //
          DEDENT,     //
          ID_LOWER,   // d
          ID_LOWER,   // e
          ID_LOWER,   // f
          NEWLINE,    //
          INDENT,     //
          ID_LOWER,   // x
          ID_LOWER,   // y
          ID_LOWER,   // z
          NEWLINE,    //
          DEDENT,     //
          LANG_END,   //
      };
      CHECK(frag_cat == expected_fragment_categories);
    }
    SECTION("error: unmatched language")
    {
      const auto text    = R"(
A⎢B «
 ⎢C⎢D
»
)";
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize_unique("..", text));
      CHECK_THAT(err_msg, ContainsSubstring("LANGUAGE started by '«' must be finished by '»'"));
    }
    SECTION("error: unmatched language")
    {
      const auto text    = R"(
A⎢B «
 ⎢C⎢D »
)";
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize_unique("..", text));
      CHECK_THAT(err_msg, ContainsSubstring("unexpected '»' at"));
    }
  }
}
