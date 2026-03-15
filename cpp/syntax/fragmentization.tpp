#include "fragmentization.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  using namespace Catch::Matchers;

  using enum codepoint_category_t;
  using enum fragment_category_t;
  using enum fragment_case_mask_t;

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
    CHECK(cct[U'A'] == XID_Start);
    CHECK(cct[U'_'] == XID_Start);
  }

  TEST_CASE("fragmentization", "[fragmentization_t]")
  {
    SECTION("error: forbidden codepoint")
    {
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", "zyẍ_\n"));
      CHECK_THAT(err_msg, ContainsSubstring("Forbidden codepoint"));
      CHECK_THAT(err_msg, ContainsSubstring("0x0308"));
    }
    SECTION("error: stray LANG_END")
    {
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", "»\n"));
      CHECK_THAT(err_msg, ContainsSubstring("Unexpected '»'"));
    }
    SECTION("error: non-matching parentheses")
    {
      const auto text    = R"(
A ⎢ ( B
)
)";
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", text));
      CHECK_THAT(err_msg, ContainsSubstring("Expected multi-line language to continue"));
    }
    SECTION("empty file")
    {
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", ""));
      CHECK_THAT(err_msg, ContainsSubstring("source-code expected to end with newline"));
      const auto frag = SILVA_REQUIRE(fragmentize("..", "\n"));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}},
          {WHITESPACE, NONE, {0, 0, 0}},
          {LANG_END, NONE, {0, 0, 0}},
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("error: newline in string")
    {
      const auto text    = R"('abc#\
xyz'
)";
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", text));
      CHECK_THAT(err_msg, ContainsSubstring("unexpected escape sequence"));
    }
    SECTION("basic")
    {
      const auto text = R"(

xyz123_äß

)";
      const auto frag = SILVA_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}},
          {WHITESPACE, NONE, {0, 0, 0}},
          {IDENTIFIER, NONE, {2, 0, 2}},
          {NEWLINE, NONE, {2, 9, 13}},
          {LANG_END, NONE, {3, 0, 14}},
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("kebab identifier")
    {
      const auto text = "hello-world -++- hello/world\n";
      const auto frag = SILVA_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}},
          {IDENTIFIER, NONE, {0, 0, 0}},
          {WHITESPACE, NONE, {0, 11, 11}},
          {OPERATOR, NONE, {0, 12, 12}},
          {OPERATOR, NONE, {0, 13, 13}},
          {OPERATOR, NONE, {0, 14, 14}},
          {OPERATOR, NONE, {0, 15, 15}},
          {WHITESPACE, NONE, {0, 16, 16}},
          {IDENTIFIER, NONE, {0, 17, 17}},
          {OPERATOR, NONE, {0, 22, 22}},
          {IDENTIFIER, NONE, {0, 23, 23}},
          {NEWLINE, NONE, {0, 28, 28}},
          {LANG_END, NONE, {0, 28, 28}},
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("error: kebab identifier")
    {
      const auto text    = "h-w h-w-\n";
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", text));
      CHECK_THAT(err_msg, ContainsSubstring("Identifier"));
      CHECK_THAT(err_msg, ContainsSubstring("may not end with '-'"));
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
      const auto frag = SILVA_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}},   //
          {WHITESPACE, NONE, {0, 0, 0}},   //
          {IDENTIFIER, NONE, {1, 0, 1}},   // def
          {NEWLINE, NONE, {1, 3, 4}},      //
          {INDENT, NONE, {2, 2, 7}},       //
          {IDENTIFIER, NONE, {2, 2, 7}},   // test
          {WHITESPACE, NONE, {2, 6, 11}},  //
          {OPERATOR, NONE, {2, 7, 12}},    // <
          {OPERATOR, NONE, {2, 8, 13}},    // >
          {WHITESPACE, NONE, {2, 9, 14}},  //
          {NUMBER, NONE, {2, 12, 17}},     // 0x0308
          {OPERATOR, NONE, {2, 18, 23}},   // ⊙
          {STRING, NONE, {2, 19, 26}},     // 'abc'
          {NEWLINE, NONE, {2, 24, 31}},    //
          {STRING, NONE, {3, 2, 34}},      // "abc"
          {NEWLINE, NONE, {3, 7, 39}},     //
          {INDENT, NONE, {4, 4, 44}},      //
          {IDENTIFIER, NONE, {4, 4, 44}},  // deep
          {NEWLINE, NONE, {4, 8, 48}},     //
          {COMMENT, NONE, {6, 2, 53}},     // # Comment
          {WHITESPACE, NONE, {6, 11, 62}}, //
          {DEDENT, NONE, {8, 0, 64}},      //
          {DEDENT, NONE, {8, 0, 64}},      //
          {IDENTIFIER, NONE, {8, 0, 64}},  // back
          {NEWLINE, NONE, {8, 4, 68}},     //
          {LANG_END, NONE, {9, 0, 69}},    //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("init indent")
    {
      const auto text = "  import\n";
      const auto frag = SILVA_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}}, //
          {INDENT, NONE, {0, 2, 2}},     //
          {IDENTIFIER, NONE, {0, 2, 2}}, // import
          {NEWLINE, NONE, {0, 8, 8}},    //
          {DEDENT, NONE, {0, 8, 8}},     //
          {LANG_END, NONE, {0, 8, 8}},   //
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
      const auto frag = SILVA_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}},   //
          {WHITESPACE, NONE, {0, 0, 0}},   //
          {IDENTIFIER, NONE, {1, 0, 1}},   // def
          {NEWLINE, NONE, {1, 3, 4}},      //
          {INDENT, NONE, {2, 4, 9}},       //
          {IDENTIFIER, NONE, {2, 4, 9}},   // id
          {WHITESPACE, NONE, {2, 6, 11}},  //
          {COMMENT, NONE, {2, 7, 12}},     //
          {NEWLINE, NONE, {2, 11, 16}},    //
          {INDENT, NONE, {3, 8, 25}},      //
          {IDENTIFIER, NONE, {3, 8, 25}},  // id
          {WHITESPACE, NONE, {3, 10, 27}}, //
          {PAREN_LEFT, NONE, {3, 11, 28}}, // (
          {WHITESPACE, NONE, {3, 12, 29}}, //
          {IDENTIFIER, NONE, {4, 0, 30}},  // b
          {WHITESPACE, NONE, {4, 1, 31}},  //
          {COMMENT, NONE, {4, 5, 35}},     // # Hi
          {WHITESPACE, NONE, {4, 9, 39}},  //
          {IDENTIFIER, NONE, {5, 2, 42}},  // c
          {WHITESPACE, NONE, {5, 3, 43}},  //
          {PAREN_RIGHT, NONE, {6, 0, 44}}, // )
          {NEWLINE, NONE, {6, 1, 45}},     //
          {DEDENT, NONE, {7, 4, 50}},      //
          {IDENTIFIER, NONE, {7, 4, 50}},  // id
          {NEWLINE, NONE, {7, 6, 52}},     //
          {DEDENT, NONE, {7, 6, 52}},      //
          {LANG_END, NONE, {7, 6, 52}},    //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("line continuation")
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
      const auto frag = SILVA_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}},  //
          {WHITESPACE, NONE, {0, 0, 0}},  //
          {IDENTIFIER, NONE, {1, 0, 1}},  // def
          {WHITESPACE, NONE, {1, 3, 4}},  //
          {COMMENT, NONE, {1, 4, 5}},     // '# Hi \'
          {NEWLINE, NONE, {1, 10, 11}},   //
          {INDENT, NONE, {2, 2, 14}},     //
          {STRING, NONE, {2, 2, 14}},     // 'ab\'c#xyz'
          {NEWLINE, NONE, {2, 13, 25}},   //
          {IDENTIFIER, NONE, {3, 2, 28}}, // var
          {STRING, NONE, {3, 5, 31}},     // 'abc#\nxy¶z'
          {NEWLINE, NONE, {4, 10, 50}},   //
          {IDENTIFIER, NONE, {5, 2, 53}}, // retval
          {WHITESPACE, NONE, {5, 8, 59}}, //
          {IDENTIFIER, NONE, {6, 0, 62}}, // y
          {NEWLINE, NONE, {6, 1, 63}},    //
          {IDENTIFIER, NONE, {7, 2, 66}}, // retval
          {WHITESPACE, NONE, {7, 8, 72}}, //
          {IDENTIFIER, NONE, {8, 0, 74}}, // y
          {NEWLINE, NONE, {8, 1, 75}},    //
          {DEDENT, NONE, {8, 1, 75}},     //
          {LANG_END, NONE, {8, 1, 75}},   //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("two multi-line strings")
    {
      const auto text = R"(
x¶ abc
y¶ xyz
)";
      const auto frag = SILVA_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}}, //
          {WHITESPACE, NONE, {0, 0, 0}}, //
          {IDENTIFIER, NONE, {1, 0, 1}}, // x
          {STRING, NONE, {1, 1, 2}},     // 'abc'
          {NEWLINE, NONE, {1, 6, 8}},    //
          {IDENTIFIER, NONE, {2, 0, 9}}, // y
          {STRING, NONE, {2, 1, 10}},    // 'xyz'
          {NEWLINE, NONE, {2, 6, 16}},   //
          {LANG_END, NONE, {2, 6, 16}},  //
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
      const auto frag = SILVA_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}},    //
          {WHITESPACE, NONE, {0, 0, 0}},    //
          {IDENTIFIER, NONE, {1, 0, 1}},    // Python
          {WHITESPACE, NONE, {1, 6, 7}},    //
          {LANG_BEGIN, NONE, {1, 7, 8}},    // ⎢
          {IDENTIFIER, NONE, {1, 8, 11}},   // def
          {NEWLINE, NONE, {1, 11, 14}},     //
          {INDENT, NONE, {2, 10, 27}},      //
          {IDENTIFIER, NONE, {2, 10, 27}},  // return
          {WHITESPACE, NONE, {2, 16, 33}},  //
          {PAREN_LEFT, NONE, {2, 17, 34}},  // (
          {IDENTIFIER, NONE, {2, 18, 35}},  // x
          {WHITESPACE, NONE, {2, 19, 36}},  //
          {OPERATOR, NONE, {2, 20, 37}},    // +
          {WHITESPACE, NONE, {2, 21, 38}},  //
          {IDENTIFIER, NONE, {3, 9, 50}},   // y
          {PAREN_RIGHT, NONE, {3, 10, 51}}, // )
          {NEWLINE, NONE, {3, 11, 52}},     //
          {DEDENT, NONE, {4, 0, 53}},       //
          {LANG_END, NONE, {4, 0, 53}},     //
          {NEWLINE, NONE, {4, 0, 53}},      //
          {IDENTIFIER, NONE, {5, 0, 54}},   // Python
          {WHITESPACE, NONE, {5, 6, 60}},   //
          {LANG_BEGIN, NONE, {5, 7, 61}},   // «
          {WHITESPACE, NONE, {5, 8, 63}},   //
          {IDENTIFIER, NONE, {6, 0, 64}},   // def
          {NEWLINE, NONE, {6, 3, 67}},      //
          {INDENT, NONE, {7, 2, 70}},       //
          {IDENTIFIER, NONE, {7, 2, 70}},   // return
          {WHITESPACE, NONE, {7, 8, 76}},   //
          {PAREN_LEFT, NONE, {7, 9, 77}},   // (
          {WHITESPACE, NONE, {7, 10, 78}},  //
          {IDENTIFIER, NONE, {8, 0, 79}},   // x
          {PAREN_RIGHT, NONE, {8, 1, 80}},  // )
          {NEWLINE, NONE, {8, 2, 81}},      //
          {DEDENT, NONE, {9, 0, 82}},       //
          {LANG_END, NONE, {9, 0, 82}},     // «
          {NEWLINE, NONE, {9, 1, 84}},      //
          {LANG_END, NONE, {9, 1, 84}},     //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("complex language 1")
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
      const auto frag = SILVA_REQUIRE(fragmentize("..", text));
      const array_t<fragment_t> expected_fragments{
          {LANG_BEGIN, NONE, {0, 0, 0}},    //
          {WHITESPACE, NONE, {0, 0, 0}},    //
          {IDENTIFIER, NONE, {1, 0, 1}},    // Python
          {WHITESPACE, NONE, {1, 6, 7}},    //
          {LANG_BEGIN, NONE, {1, 7, 8}},    // ⎢
          {IDENTIFIER, NONE, {1, 8, 11}},   // def
          {NEWLINE, NONE, {1, 11, 14}},     //
          {INDENT, NONE, {3, 10, 38}},      //
          {LANG_BEGIN, NONE, {3, 10, 38}},  // ⎢
          {INDENT, NONE, {3, 12, 42}},      //
          {IDENTIFIER, NONE, {3, 12, 42}},  // str
          {WHITESPACE, NONE, {3, 15, 45}},  //
          {STRING, NONE, {3, 16, 46}},      // ¶Hello...
          {NEWLINE, NONE, {4, 33, 95}},     //
          {DEDENT, NONE, {5, 8, 106}},      //
          {LANG_END, NONE, {5, 8, 106}},    //
          {NEWLINE, NONE, {5, 8, 106}},     //
          {LANG_BEGIN, NONE, {6, 10, 119}}, //
          {IDENTIFIER, NONE, {6, 11, 122}}, // int
          {WHITESPACE, NONE, {6, 14, 125}}, //
          {LANG_BEGIN, NONE, {6, 15, 126}}, // «
          {WHITESPACE, NONE, {6, 16, 128}}, //
          {INDENT, NONE, {7, 15, 148}},     //
          {IDENTIFIER, NONE, {7, 15, 148}}, // x
          {WHITESPACE, NONE, {7, 16, 149}}, //
          {NEWLINE, NONE, {7, 17, 150}},    //
          {DEDENT, NONE, {7, 17, 150}},     //
          {LANG_END, NONE, {7, 17, 150}},   // »
          {NEWLINE, NONE, {7, 18, 152}},    //
          {LANG_END, NONE, {7, 18, 152}},   //
          {NEWLINE, NONE, {7, 18, 152}},    //
          {DEDENT, NONE, {7, 18, 152}},     //
          {LANG_END, NONE, {7, 18, 152}},   //
          {NEWLINE, NONE, {7, 18, 152}},    //
          {LANG_END, NONE, {7, 18, 152}},   //
      };
      CHECK(frag->fragments == expected_fragments);
    }
    SECTION("complex language 2")
    {
      const auto text     = R"(
A ⎢ B «
  ⎢  C ⎢ D
  ⎢    ⎢  E ¶ abc
  ⎢    ⎢    ¶ xyz
  ⎢    ⎢
  ⎢  F »
)";
      const auto frag     = SILVA_REQUIRE(fragmentize("..", text));
      const auto frag_cat = only_real_categories(frag->fragments);
      const array_t<fragment_category_t> expected_fragment_categories{
          LANG_BEGIN, //
          IDENTIFIER, // A
          LANG_BEGIN, //
          INDENT,     //
          IDENTIFIER, // B
          LANG_BEGIN, //
          INDENT,     //
          IDENTIFIER, // C
          LANG_BEGIN, //
          INDENT,     //
          IDENTIFIER, // D
          NEWLINE,    //
          INDENT,     //
          IDENTIFIER, // E
          STRING,     //
          NEWLINE,    //
          DEDENT,     //
          DEDENT,     //
          LANG_END,   //
          NEWLINE,    //
          IDENTIFIER, // F
          NEWLINE,    //
          DEDENT,     //
          LANG_END,   //
          NEWLINE,    //
          DEDENT,     //
          LANG_END,   //
          NEWLINE,    //
          LANG_END,   //
      };
      CHECK(frag_cat == expected_fragment_categories);
    }
    SECTION("complex language 3")
    {
      const auto text     = R"(
A ⎢ B « C » D
)";
      const auto frag     = SILVA_REQUIRE(fragmentize("..", text));
      const auto frag_cat = only_real_categories(frag->fragments);
      const array_t<fragment_category_t> expected_fragment_categories{
          LANG_BEGIN, //
          IDENTIFIER, // A
          LANG_BEGIN, //
          INDENT,     //
          IDENTIFIER, // B
          LANG_BEGIN, // «
          INDENT,     //
          IDENTIFIER, // C
          NEWLINE,    //
          DEDENT,     //
          LANG_END,   // »
          IDENTIFIER, // D
          NEWLINE,    //
          DEDENT,     //
          LANG_END,   //
          NEWLINE,    //
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
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", text));
      CHECK_THAT(err_msg, ContainsSubstring("LANGUAGE started by '«' must be finished by '»'"));
    }
    SECTION("error: unmatched language")
    {
      const auto text    = R"(
A⎢B «
 ⎢C⎢D »
)";
      const auto err_msg = SILVA_REQUIRE_ERROR(fragmentize("..", text));
      CHECK_THAT(err_msg, ContainsSubstring("Unexpected '»' at"));
    }
  }
}
