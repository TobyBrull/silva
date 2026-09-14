#include "fragmentization.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  using namespace Catch::Matchers;

  using enum codepoint_category_t;
  using enum fragment_category_t;

  array_t<fragment_category_t> only_categories(const array_t<fragment_t>& x)
  {
    array_t<fragment_category_t> retval;
    retval.reserve(x.size());
    for (const auto& elem: x) {
      retval.push_back(elem.category);
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
    SECTION("error: line-continuation leaving multi-line language")
    {
      const auto text    = R"(
A ⎢ ( B \
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
          {NEWLINE, {0, 0, 0}},
          {LANG_END, {1, 0, 1}},
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("basic")
    {
      const auto text = R"(

xyz123_äß

)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},
          {NEWLINE, {0, 0, 0}},
          {NEWLINE, {1, 0, 1}},
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
          {NEWLINE, {3, 0, 14}},
          {LANG_END, {4, 0, 15}},
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
          {LANG_END, {1, 0, 17}},
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
          {LANG_BEGIN, {0, 0, 0}}, //
          {NEWLINE, {0, 0, 0}},    //
          {ID_LOWER, {1, 0, 1}},   // d
          {ID_LOWER, {1, 1, 2}},   // e
          {ID_LOWER, {1, 2, 3}},   // f
          {NEWLINE, {1, 3, 4}},    //
          {INDENT, {2, 0, 5}},     //
          {ID_LOWER, {2, 2, 7}},   // t
          {ID_LOWER, {2, 3, 8}},   // e
          {ID_LOWER, {2, 4, 9}},   // s
          {ID_LOWER, {2, 5, 10}},  // t
          {SPACE, {2, 6, 11}},     //
          {OPERATOR, {2, 7, 12}},  // <
          {OPERATOR, {2, 8, 13}},  // >
          {SPACE, {2, 9, 14}},     //
          {SPACE, {2, 10, 15}},    //
          {SPACE, {2, 11, 16}},    //
          {DIGIT, {2, 12, 17}},    // 0
          {ID_LOWER, {2, 13, 18}}, // x
          {DIGIT, {2, 14, 19}},    // 0
          {DIGIT, {2, 15, 20}},    // 3
          {DIGIT, {2, 16, 21}},    // 0
          {DIGIT, {2, 17, 22}},    // 8
          {OPERATOR, {2, 18, 23}}, // ⊙
          {OPERATOR, {2, 19, 26}}, // '
          {ID_LOWER, {2, 20, 27}}, // a
          {ID_LOWER, {2, 21, 28}}, // b
          {ID_LOWER, {2, 22, 29}}, // c
          {OPERATOR, {2, 23, 30}}, // '
          {NEWLINE, {2, 24, 31}},  //
          {OPERATOR, {3, 2, 34}},  // \"
          {ID_LOWER, {3, 3, 35}},  // a
          {ID_LOWER, {3, 4, 36}},  // b
          {ID_LOWER, {3, 5, 37}},  // c
          {OPERATOR, {3, 6, 38}},  // \"
          {NEWLINE, {3, 7, 39}},   //
          {INDENT, {4, 0, 40}},    //
          {ID_LOWER, {4, 4, 44}},  // d
          {ID_LOWER, {4, 5, 45}},  // e
          {ID_LOWER, {4, 6, 46}},  // e
          {ID_LOWER, {4, 7, 47}},  // p
          {NEWLINE, {4, 8, 48}},   //
          {NEWLINE, {5, 1, 50}},   //
          {DEDENT, {6, 0, 51}},    //
          {OPERATOR, {6, 2, 53}},  // #
          {SPACE, {6, 3, 54}},     //
          {ID_UPPER, {6, 4, 55}},  // C
          {ID_LOWER, {6, 5, 56}},  // o
          {ID_LOWER, {6, 6, 57}},  // m
          {ID_LOWER, {6, 7, 58}},  // m
          {ID_LOWER, {6, 8, 59}},  // e
          {ID_LOWER, {6, 9, 60}},  // n
          {ID_LOWER, {6, 10, 61}}, // t
          {NEWLINE, {6, 11, 62}},  //
          {NEWLINE, {7, 0, 63}},   //
          {DEDENT, {8, 0, 64}},    //
          {ID_LOWER, {8, 0, 64}},  // b
          {ID_LOWER, {8, 1, 65}},  // a
          {ID_LOWER, {8, 2, 66}},  // c
          {ID_LOWER, {8, 3, 67}},  // k
          {NEWLINE, {8, 4, 68}},   //
          {NEWLINE, {9, 0, 69}},   //
          {LANG_END, {10, 0, 70}}, //
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
          {DEDENT, {1, 0, 6}},     //
          {LANG_END, {1, 0, 6}},   //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("broken-indent")
    {
      const auto text = R"(
def
    id
  fed
)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},          //
          {NEWLINE, {0, 0, 0}},             //
          {ID_LOWER, {1, 0, 1}},            // d
          {ID_LOWER, {1, 1, 2}},            // e
          {ID_LOWER, {1, 2, 3}},            // f
          {NEWLINE, {1, 3, 4}},             //
          {INDENT, {2, 0, 5}},              //
          {ID_LOWER, {2, 4, 9}},            // i
          {ID_LOWER, {2, 5, 10}},           // d
          {NEWLINE, {2, 6, 11}},            //
          {DEDENT, {3, 0, 12}},             //
          {INDENTATION_BROKEN, {3, 0, 12}}, //
          {ID_LOWER, {3, 2, 14}},           // f
          {ID_LOWER, {3, 3, 15}},           // e
          {ID_LOWER, {3, 4, 16}},           // d
          {NEWLINE, {3, 5, 17}},            //
          {LANG_END, {4, 0, 18}},           //
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
          {NEWLINE, {0, 0, 0}},       //
          {ID_LOWER, {1, 0, 1}},      // d
          {ID_LOWER, {1, 1, 2}},      // e
          {ID_LOWER, {1, 2, 3}},      // f
          {NEWLINE, {1, 3, 4}},       //
          {INDENT, {2, 0, 5}},        //
          {ID_LOWER, {2, 4, 9}},      // i
          {ID_LOWER, {2, 5, 10}},     // d
          {SPACE, {2, 6, 11}},        //
          {OPERATOR, {2, 7, 12}},     // #
          {SPACE, {2, 8, 13}},        //
          {ID_UPPER, {2, 9, 14}},     // H
          {ID_LOWER, {2, 10, 15}},    // o
          {NEWLINE, {2, 11, 16}},     //
          {INDENT, {3, 0, 17}},       //
          {ID_LOWER, {3, 8, 25}},     // i
          {ID_LOWER, {3, 9, 26}},     // d
          {SPACE, {3, 10, 27}},       //
          {PARENTHESIS, {3, 11, 28}}, // (
          {NEWLINE, {3, 12, 29}},     //
          {DEDENT, {4, 0, 30}},       //
          {DEDENT, {4, 0, 30}},       //
          {ID_LOWER, {4, 0, 30}},     // b
          {SPACE, {4, 1, 31}},        //
          {SPACE, {4, 2, 32}},        //
          {SPACE, {4, 3, 33}},        //
          {SPACE, {4, 4, 34}},        //
          {OPERATOR, {4, 5, 35}},     // #
          {SPACE, {4, 6, 36}},        //
          {ID_UPPER, {4, 7, 37}},     // H
          {ID_LOWER, {4, 8, 38}},     // i
          {NEWLINE, {4, 9, 39}},      //
          {INDENT, {5, 0, 40}},       //
          {ID_LOWER, {5, 2, 42}},     // c
          {NEWLINE, {5, 3, 43}},      //
          {DEDENT, {6, 0, 44}},       //
          {PARENTHESIS, {6, 0, 44}},  // )
          {NEWLINE, {6, 1, 45}},      //
          {INDENT, {7, 0, 46}},       //
          {ID_LOWER, {7, 4, 50}},     // i
          {ID_LOWER, {7, 5, 51}},     // d
          {NEWLINE, {7, 6, 52}},      //
          {DEDENT, {8, 0, 53}},       //
          {LANG_END, {8, 0, 53}},     //
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
          {LANG_BEGIN, {0, 0, 0}},         //
          {NEWLINE, {0, 0, 0}},            //
          {ID_LOWER, {1, 0, 1}},           // d
          {ID_LOWER, {1, 1, 2}},           // e
          {ID_LOWER, {1, 2, 3}},           // f
          {SPACE, {1, 3, 4}},              //
          {OPERATOR, {1, 4, 5}},           // #
          {SPACE, {1, 5, 6}},              //
          {ID_UPPER, {1, 6, 7}},           // H
          {ID_LOWER, {1, 7, 8}},           // i
          {SPACE, {1, 8, 9}},              //
          {LINE_CONTINUATION, {1, 9, 10}}, //
          {SPACE, {2, 0, 12}},             //
          {SPACE, {2, 1, 13}},             //
          {OPERATOR, {2, 2, 14}},          // '
          {ID_LOWER, {2, 3, 15}},          // a
          {ID_LOWER, {2, 4, 16}},          // b
          {OPERATOR, {2, 5, 17}},          // backslash
          {OPERATOR, {2, 6, 18}},          // '
          {ID_LOWER, {2, 7, 19}},          // c
          {OPERATOR, {2, 8, 20}},          // #
          {ID_LOWER, {2, 9, 21}},          // x
          {ID_LOWER, {2, 10, 22}},         // y
          {ID_LOWER, {2, 11, 23}},         // z
          {OPERATOR, {2, 12, 24}},         // '
          {NEWLINE, {2, 13, 25}},          //
          {INDENT, {3, 0, 26}},            //
          {ID_LOWER, {3, 2, 28}},          // v
          {ID_LOWER, {3, 3, 29}},          // a
          {ID_LOWER, {3, 4, 30}},          // r
          {MULTILINE_STRING, {3, 5, 31}},  // ¶abc#\n     ¶xy¶z
          {NEWLINE, {4, 10, 50}},          //
          {ID_LOWER, {5, 2, 53}},          // r
          {ID_LOWER, {5, 3, 54}},          // e
          {ID_LOWER, {5, 4, 55}},          // t
          {ID_LOWER, {5, 5, 56}},          // v
          {ID_LOWER, {5, 6, 57}},          // a
          {ID_LOWER, {5, 7, 58}},          // l
          {SPACE, {5, 8, 59}},             //
          {LINE_CONTINUATION, {5, 9, 60}}, //
          {ID_LOWER, {6, 0, 62}},          // y
          {NEWLINE, {6, 1, 63}},           //
          {ID_LOWER, {7, 2, 66}},          // r
          {ID_LOWER, {7, 3, 67}},          // e
          {ID_LOWER, {7, 4, 68}},          // t
          {ID_LOWER, {7, 5, 69}},          // v
          {ID_LOWER, {7, 6, 70}},          // a
          {ID_LOWER, {7, 7, 71}},          // l
          {LINE_CONTINUATION, {7, 8, 72}}, //
          {ID_LOWER, {8, 0, 74}},          // y
          {NEWLINE, {8, 1, 75}},           //
          {DEDENT, {9, 0, 76}},            //
          {LANG_END, {9, 0, 76}},          //
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
          {LANG_BEGIN, {0, 0, 0}},        //
          {NEWLINE, {0, 0, 0}},           //
          {ID_LOWER, {1, 0, 1}},          // x
          {MULTILINE_STRING, {1, 1, 2}},  // ' abc'
          {NEWLINE, {1, 6, 8}},           //
          {ID_LOWER, {2, 0, 9}},          // y
          {MULTILINE_STRING, {2, 1, 10}}, // ' xyz'
          {NEWLINE, {2, 6, 16}},          //
          {LANG_END, {3, 0, 17}},         //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("language")
    {
      const auto text = R"(
Python ⎢def
       ⎢  return (x + \
       ⎢ y)

Python «
def
  return ( \
x)
»
)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},          //
          {NEWLINE, {0, 0, 0}},             //
          {ID_UPPER, {1, 0, 1}},            // P
          {ID_LOWER, {1, 1, 2}},            // y
          {ID_LOWER, {1, 2, 3}},            // t
          {ID_LOWER, {1, 3, 4}},            // h
          {ID_LOWER, {1, 4, 5}},            // o
          {ID_LOWER, {1, 5, 6}},            // n
          {SPACE, {1, 6, 7}},               //
          {LANG_BEGIN, {1, 7, 8}},          // ⎢
          {ID_LOWER, {1, 8, 11}},           // d
          {ID_LOWER, {1, 9, 12}},           // e
          {ID_LOWER, {1, 10, 13}},          // f
          {NEWLINE, {1, 11, 14}},           //
          {INDENT, {2, 8, 25}},             //
          {ID_LOWER, {2, 10, 27}},          // r
          {ID_LOWER, {2, 11, 28}},          // e
          {ID_LOWER, {2, 12, 29}},          // t
          {ID_LOWER, {2, 13, 30}},          // u
          {ID_LOWER, {2, 14, 31}},          // r
          {ID_LOWER, {2, 15, 32}},          // n
          {SPACE, {2, 16, 33}},             //
          {PARENTHESIS, {2, 17, 34}},       // (
          {ID_LOWER, {2, 18, 35}},          // x
          {SPACE, {2, 19, 36}},             //
          {OPERATOR, {2, 20, 37}},          // +
          {SPACE, {2, 21, 38}},             //
          {LINE_CONTINUATION, {2, 22, 39}}, //
          {SPACE, {3, 8, 51}},              //
          {ID_LOWER, {3, 9, 52}},           // y
          {PARENTHESIS, {3, 10, 53}},       // )
          {NEWLINE, {3, 11, 54}},           //
          {DEDENT, {4, 0, 55}},             //
          {LANG_END, {4, 0, 55}},           //
          {NEWLINE, {4, 0, 55}},            //
          {NEWLINE, {4, 0, 55}},            //
          {ID_UPPER, {5, 0, 56}},           // P
          {ID_LOWER, {5, 1, 57}},           // y
          {ID_LOWER, {5, 2, 58}},           // t
          {ID_LOWER, {5, 3, 59}},           // h
          {ID_LOWER, {5, 4, 60}},           // o
          {ID_LOWER, {5, 5, 61}},           // n
          {SPACE, {5, 6, 62}},              //
          {LANG_BEGIN, {5, 7, 63}},         // «
          {NEWLINE, {5, 8, 65}},            //
          {ID_LOWER, {6, 0, 66}},           // d
          {ID_LOWER, {6, 1, 67}},           // e
          {ID_LOWER, {6, 2, 68}},           // f
          {NEWLINE, {6, 3, 69}},            //
          {INDENT, {7, 0, 70}},             //
          {ID_LOWER, {7, 2, 72}},           // r
          {ID_LOWER, {7, 3, 73}},           // e
          {ID_LOWER, {7, 4, 74}},           // t
          {ID_LOWER, {7, 5, 75}},           // u
          {ID_LOWER, {7, 6, 76}},           // r
          {ID_LOWER, {7, 7, 77}},           // n
          {SPACE, {7, 8, 78}},              //
          {PARENTHESIS, {7, 9, 79}},        // (
          {SPACE, {7, 10, 80}},             //
          {LINE_CONTINUATION, {7, 11, 81}}, //
          {ID_LOWER, {8, 0, 83}},           // x
          {PARENTHESIS, {8, 1, 84}},        // )
          {NEWLINE, {8, 2, 85}},            //
          {NEWLINE, {9, 0, 86}},            //
          {DEDENT, {9, 0, 86}},             //
          {LANG_END, {9, 0, 86}},           // »
          {NEWLINE, {9, 1, 88}},            //
          {LANG_END, {10, 0, 89}},          //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("language-parens")
    {
      const auto text = R"(
Py ⎢ (x + \
   ⎢ ⎢ y
   ⎢ )
)";
      const auto frag = SILVA_REQUIRE(fragmentize_unique("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, {0, 0, 0}},          //
          {NEWLINE, {0, 0, 0}},             //
          {ID_UPPER, {1, 0, 1}},            // P
          {ID_LOWER, {1, 1, 2}},            // y
          {SPACE, {1, 2, 3}},               //
          {LANG_BEGIN, {1, 3, 4}},          // ⎢
          {INDENT, {1, 4, 7}},              //
          {PARENTHESIS, {1, 5, 8}},         // (
          {ID_LOWER, {1, 6, 9}},            // x
          {SPACE, {1, 7, 10}},              //
          {OPERATOR, {1, 8, 11}},           // +
          {SPACE, {1, 9, 12}},              //
          {LINE_CONTINUATION, {1, 10, 13}}, //
          {SPACE, {2, 4, 21}},              //
          {LANG_BEGIN, {2, 5, 22}},         // ⎢
          {INDENT, {2, 6, 25}},             //
          {ID_LOWER, {2, 7, 26}},           // y
          {NEWLINE, {2, 8, 27}},            //
          {DEDENT, {3, 5, 35}},             //
          {LANG_END, {3, 5, 35}},           //
          {NEWLINE, {3, 5, 35}},            //
          {PARENTHESIS, {3, 5, 35}},        // )
          {NEWLINE, {3, 6, 36}},            //
          {DEDENT, {4, 0, 37}},             //
          {LANG_END, {4, 0, 37}},           //
          {NEWLINE, {4, 0, 37}},            //
          {NEWLINE, {4, 0, 37}},            //
          {LANG_END, {4, 0, 37}},           //
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
          {LANG_BEGIN, {0, 0, 0}},         //
          {NEWLINE, {0, 0, 0}},            //
          {ID_UPPER, {1, 0, 1}},           // P
          {ID_LOWER, {1, 1, 2}},           // y
          {ID_LOWER, {1, 2, 3}},           // t
          {ID_LOWER, {1, 3, 4}},           // h
          {ID_LOWER, {1, 4, 5}},           // o
          {ID_LOWER, {1, 5, 6}},           // n
          {SPACE, {1, 6, 7}},              //
          {LANG_BEGIN, {1, 7, 8}},         //
          {ID_LOWER, {1, 8, 11}},          // d
          {ID_LOWER, {1, 9, 12}},          // e
          {ID_LOWER, {1, 10, 13}},         // f
          {NEWLINE, {1, 11, 14}},          //
          {NEWLINE, {2, 8, 25}},           //
          {INDENT, {3, 8, 36}},            //
          {LANG_BEGIN, {3, 10, 38}},       //
          {INDENT, {3, 11, 41}},           //
          {ID_LOWER, {3, 12, 42}},         // s
          {ID_LOWER, {3, 13, 43}},         // t
          {ID_LOWER, {3, 14, 44}},         // r
          {SPACE, {3, 15, 45}},            //
          {MULTILINE_STRING, {3, 16, 46}}, //
          {NEWLINE, {4, 33, 95}},          //
          {DEDENT, {5, 8, 106}},           //
          {LANG_END, {5, 8, 106}},         //
          {NEWLINE, {5, 8, 106}},          //
          {NEWLINE, {5, 8, 106}},          //
          {LANG_BEGIN, {6, 10, 119}},      //
          {ID_LOWER, {6, 11, 122}},        // i
          {ID_LOWER, {6, 12, 123}},        // n
          {ID_LOWER, {6, 13, 124}},        // t
          {SPACE, {6, 14, 125}},           //
          {LANG_BEGIN, {6, 15, 126}},      //
          {NEWLINE, {6, 16, 128}},         //
          {INDENT, {7, 11, 144}},          //
          {ID_LOWER, {7, 15, 148}},        // x
          {SPACE, {7, 16, 149}},           //
          {NEWLINE, {7, 17, 150}},         //
          {DEDENT, {7, 17, 150}},          //
          {LANG_END, {7, 17, 150}},        //
          {NEWLINE, {7, 18, 152}},         //
          {LANG_END, {8, 0, 153}},         //
          {NEWLINE, {8, 0, 153}},          //
          {NEWLINE, {8, 0, 153}},          //
          {DEDENT, {8, 0, 153}},           //
          {LANG_END, {8, 0, 153}},         //
          {NEWLINE, {8, 0, 153}},          //
          {NEWLINE, {8, 0, 153}},          //
          {LANG_END, {8, 0, 153}},         //
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
      const auto frag_cat = only_categories(frag->fragments);
      const array_t<fragment_category_t> expected_fragment_categories{
          LANG_BEGIN,       //
          NEWLINE,          //
          ID_UPPER,         // A
          SPACE,            //
          LANG_BEGIN,       //
          INDENT,           //
          ID_UPPER,         // B
          SPACE,            //
          LANG_BEGIN,       //
          NEWLINE,          //
          INDENT,           //
          ID_UPPER,         // C
          SPACE,            //
          LANG_BEGIN,       //
          INDENT,           //
          ID_UPPER,         // D
          NEWLINE,          //
          INDENT,           //
          ID_UPPER,         // E
          SPACE,            //
          MULTILINE_STRING, //
          NEWLINE,          //
          NEWLINE,          //
          DEDENT,           //
          DEDENT,           //
          LANG_END,         //
          NEWLINE,          //
          ID_UPPER,         // F
          SPACE,            //
          NEWLINE,          //
          DEDENT,           //
          LANG_END,         //
          SPACE,            //
          NEWLINE,          //
          DEDENT,           //
          LANG_END,         //
          NEWLINE,          //
          NEWLINE,          //
          LANG_END,         //
      };
      CHECK(frag_cat == expected_fragment_categories);
    }
    SECTION("nested-language-3")
    {
      const auto text     = R"(
A ⎢ B « C » D
)";
      const auto frag     = SILVA_REQUIRE(fragmentize_unique("..", text));
      const auto frag_cat = only_categories(frag->fragments);
      const array_t<fragment_category_t> expected_fragment_categories{
          LANG_BEGIN, //
          NEWLINE,    //
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
      const auto frag_cat = only_categories(frag->fragments);
      const array_t<fragment_category_t> expected_fragment_categories{
          LANG_BEGIN, //
          NEWLINE,    //
          ID_LOWER,   // d
          ID_LOWER,   // e
          ID_LOWER,   // f
          NEWLINE,    //
          INDENT,     //
          ID_LOWER,   // a
          ID_LOWER,   // b
          ID_LOWER,   // c
          NEWLINE,    //
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
    SECTION("comments")
    {
      const auto text     = R"(

# a

A
  B\
C # b

  # c

  D

# d

E
)";
      const auto frag     = SILVA_REQUIRE(fragmentize_unique("..", text));
      const auto frag_cat = only_categories(frag->fragments);
      const array_t<fragment_category_t> expected_fragment_categories{
          LANG_BEGIN,        //
                             //
          NEWLINE,           //
          NEWLINE,           //
          OPERATOR,          // #
          SPACE,             //
          ID_LOWER,          // a
          NEWLINE,           //
          NEWLINE,           //
                             //
          ID_UPPER,          // A
          NEWLINE,           //
          INDENT,            //
          ID_UPPER,          // B
          LINE_CONTINUATION, //
          ID_UPPER,          // C
                             //
          SPACE,             //
          OPERATOR,          // #
          SPACE,             //
          ID_LOWER,          // b
          NEWLINE,           //
          NEWLINE,           //
          OPERATOR,          // #
          SPACE,             //
          ID_LOWER,          // c
          NEWLINE,           //
          NEWLINE,           //
                             //
          ID_UPPER,          // D
          NEWLINE,           //
          NEWLINE,           //
          DEDENT,            //
                             //
          OPERATOR,          // #
          SPACE,             //
          ID_LOWER,          // d
          NEWLINE,           //
          NEWLINE,           //
          ID_UPPER,          // E
          NEWLINE,           //
          LANG_END,          //
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
