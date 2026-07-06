#include "seed_axe.hpp"

#include "syntax.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using namespace silva::seed::impl;
using enum silva::seed::impl::assoc_t;

namespace silva::seed::test {
  using enum fragment_category_t;

  void test_axe(seed::interpreter_t& si,
                const axe_t& pa,
                const string_view_t text,
                const optional_t<string_view_t> expected_str)
  {
    INFO(text);
    auto fp       = SILVA_REQUIRE(fragmentize(si.sfp, "", string_t{text}));
    auto maybe_pt = si.apply(fp, si.sfp->name_id_of("Test"));
    optional_t<string_t> result_str;
    if (maybe_pt.has_value()) {
      auto result_pt = *std::move(maybe_pt);
      result_str     = SILVA_REQUIRE(result_pt->span().to_string());
      UNSCOPED_INFO(result_str.value());
    }
    else {
      UNSCOPED_INFO(pretty_string(maybe_pt.error()));
    }
    REQUIRE(maybe_pt.has_value() == expected_str.has_value());
    if (!expected_str.has_value()) {
      return;
    }
    CHECK(result_str.value() == expected_str.value().substr(1));
  }

  TEST_CASE("seed-axe-basic", "[seed-axe]")
  {
    syntax_farm_t sf;
    const unique_ptr_t<seed::interpreter_t> se = standard_seed_interpreter(sf.ptr());
    const string_view_t test_axe_str           = R"'(
language Test:
  ⊙ = axe Atom oper
    Dot   = rtl   infix '.'
    Sub   = ltr   postfix_nest '[' ']'
    Dol   = ltr   postfix '$'
    Exc   = ltr   postfix '!'
    Til   = rtl   prefix '~'
    Prf   = rtl   prefix '+' '-'
    Mul   = ltr   infix '*' '/'
    Add   = ltr   infix '+' '-'
    Ter   = rtl   ternary '?' ':'
    Eqa   = rtl   infix '='
  oper = operator.single | parenthesis
  Atom = identifier | number | '(' Test ')'
  skip = skip.freeForm
)'";
    SILVA_REQUIRE(se->add_seed_text("test.seed", string_t{test_axe_str}));
    const auto& sa = se->axes.at(sf.name_id_of("Test"));
    CHECK(!sa.concat_result.has_value());
    CHECK(sa.results.size() == 13);
    {
      const axe_result_t rr = sa.results.at(sf.token_id("="));
      const axe_result_t expected{
          .prefix = {none},
          .regular =
              result_oper_t<oper_regular_t>{
                  .oper       = infix_t{sf.token_id("=")},
                  .name       = sf.name_id_of("Test", "Eqa", "="),
                  .precedence = precedence_t{.level_index = 1, .assoc = RIGHT_TO_LEFT},
                  .pts        = rr.regular->pts,
              },
          .is_right_bracket = false,
      };
      CHECK(rr == expected);
    }
    {
      const axe_result_t rr = sa.results.at(sf.token_id("?"));
      const axe_result_t expected{
          .prefix = {none},
          .regular =
              result_oper_t<oper_regular_t>{
                  .oper       = ternary_t{sf.token_id("?"), sf.token_id(":")},
                  .name       = sf.name_id_of("Test", "Ter", "?"),
                  .precedence = precedence_t{.level_index = 2, .assoc = RIGHT_TO_LEFT},
                  .pts        = rr.regular->pts,
              },
          .is_right_bracket = false,
      };
      CHECK(rr == expected);
    }
    {
      const axe_result_t rr = sa.results.at(sf.token_id(":"));
      const axe_result_t expected{
          .prefix           = {none},
          .regular          = {none},
          .is_right_bracket = true,
      };
      CHECK(rr == expected);
    }
    {
      const axe_result_t rr = sa.results.at(sf.token_id("+"));
      const axe_result_t expected{
          .prefix =
              result_oper_t<oper_prefix_t>{
                  .oper       = prefix_t{sf.token_id("+")},
                  .name       = sf.name_id_of("Test", "Prf", "+"),
                  .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                  .pts        = rr.prefix->pts,
              },
          .regular =
              result_oper_t<oper_regular_t>{
                  .oper       = infix_t{sf.token_id("+")},
                  .name       = sf.name_id_of("Test", "Add", "+"),
                  .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                  .pts        = rr.regular->pts,
              },
          .is_right_bracket = false,
      };
      CHECK(rr == expected);
    }
    {
      const axe_result_t rr = sa.results.at(sf.token_id("-"));
      const axe_result_t expected{
          .prefix =
              result_oper_t<oper_prefix_t>{
                  .oper       = prefix_t{sf.token_id("-")},
                  .name       = sf.name_id_of("Test", "Prf", "-"),
                  .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                  .pts        = rr.prefix->pts,
              },
          .regular =
              result_oper_t<oper_regular_t>{
                  .oper       = infix_t{sf.token_id("-")},
                  .name       = sf.name_id_of("Test", "Add", "-"),
                  .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                  .pts        = rr.regular->pts,
              },
          .is_right_bracket = false,
      };
      CHECK(rr == expected);
    }

    test::test_axe(*se, sa, "1\n", R"(
[  0]   1:1   cat=.number                                  1

[0].Test.Atom                                     1
)");
    test::test_axe(*se, sa, "1 + 2\n", R"(
[  0]   1:1   cat=.number                                  1
[  1]   1:3   cat=.Test.oper                               +
[  2]   1:5   cat=.number                                  2

[0].Test.Add.+                                    1 + 2
  [0].Test.Atom                                   1
  [1].Test.Atom                                   2
)");
    test::test_axe(*se, sa, "1 - 2\n", R"(
[  0]   1:1   cat=.number                                  1
[  1]   1:3   cat=.Test.oper                               -
[  2]   1:5   cat=.number                                  2

[0].Test.Add.-                                    1 - 2
  [0].Test.Atom                                   1
  [1].Test.Atom                                   2
)");
    test::test_axe(*se, sa, "1 + 2 * 3 + 4\n", R"(
[  0]   1:1   cat=.number                                  1
[  1]   1:3   cat=.Test.oper                               +
[  2]   1:5   cat=.number                                  2
[  3]   1:7   cat=.Test.oper                               *
[  4]   1:9   cat=.number                                  3
[  5]   1:11  cat=.Test.oper                               +
[  6]   1:13  cat=.number                                  4

[0].Test.Add.+                                    1 + ... + 4
  [0].Test.Add.+                                  1 + 2 * 3
    [0].Test.Atom                                 1
    [1].Test.Mul.*                                2 * 3
      [0].Test.Atom                               2
      [1].Test.Atom                               3
  [1].Test.Atom                                   4
)");
    test::test_axe(*se,
                   sa,
                   "1 - 2 + f . g . h * 3 / 4\n",
                   R"(
[  0]   1:1   cat=.number                                  1
[  1]   1:3   cat=.Test.oper                               -
[  2]   1:5   cat=.number                                  2
[  3]   1:7   cat=.Test.oper                               +
[  4]   1:9   cat=.identifier                              f
[  5]   1:11  cat=.Test.oper                               .
[  6]   1:13  cat=.identifier                              g
[  7]   1:15  cat=.Test.oper                               .
[  8]   1:17  cat=.identifier                              h
[  9]   1:19  cat=.Test.oper                               *
[ 10]   1:21  cat=.number                                  3
[ 11]   1:23  cat=.Test.oper                               /
[ 12]   1:25  cat=.number                                  4

[0].Test.Add.+                                    1 - ... / 4
  [0].Test.Add.-                                  1 - 2
    [0].Test.Atom                                 1
    [1].Test.Atom                                 2
  [1].Test.Mul./                                  f . ... / 4
    [0].Test.Mul.*                                f . ... * 3
      [0].Test.Dot..                              f . g . h
        [0].Test.Atom                             f
        [1].Test.Dot..                            g . h
          [0].Test.Atom                           g
          [1].Test.Atom                           h
      [1].Test.Atom                               3
    [1].Test.Atom                                 4
)");
    test::test_axe(*se, sa, "2 ! + 3\n", R"(
[  0]   1:1   cat=.number                                  2
[  1]   1:3   cat=.Test.oper                               !
[  2]   1:5   cat=.Test.oper                               +
[  3]   1:7   cat=.number                                  3

[0].Test.Add.+                                    2 ! + 3
  [0].Test.Exc.!                                  2 !
    [0].Test.Atom                                 2
  [1].Test.Atom                                   3
)");
    test::test_axe(*se, sa, " - + 1\n", R"(
[  0]   1:2   cat=.Test.oper                               -
[  1]   1:4   cat=.Test.oper                               +
[  2]   1:6   cat=.number                                  1

[0].Test.Prf.-                                    - + 1
  [0].Test.Prf.+                                  + 1
    [0].Test.Atom                                 1
)");
    test::test_axe(*se, sa, "a + - + 1\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               +
[  2]   1:5   cat=.Test.oper                               -
[  3]   1:7   cat=.Test.oper                               +
[  4]   1:9   cat=.number                                  1

[0].Test.Add.+                                    a + - + 1
  [0].Test.Atom                                   a
  [1].Test.Prf.-                                  - + 1
    [0].Test.Prf.+                                + 1
      [0].Test.Atom                               1
)");
    test::test_axe(*se, sa, "- - 1 * 2\n", R"(
[  0]   1:1   cat=.Test.oper                               -
[  1]   1:3   cat=.Test.oper                               -
[  2]   1:5   cat=.number                                  1
[  3]   1:7   cat=.Test.oper                               *
[  4]   1:9   cat=.number                                  2

[0].Test.Mul.*                                    - - 1 * 2
  [0].Test.Prf.-                                  - - 1
    [0].Test.Prf.-                                - 1
      [0].Test.Atom                               1
  [1].Test.Atom                                   2
)");
    test::test_axe(*se, sa, "- - 1 . 2\n", R"(
[  0]   1:1   cat=.Test.oper                               -
[  1]   1:3   cat=.Test.oper                               -
[  2]   1:5   cat=.number                                  1
[  3]   1:7   cat=.Test.oper                               .
[  4]   1:9   cat=.number                                  2

[0].Test.Prf.-                                    - - 1 . 2
  [0].Test.Prf.-                                  - 1 . 2
    [0].Test.Dot..                                1 . 2
      [0].Test.Atom                               1
      [1].Test.Atom                               2
)");
    test::test_axe(*se, sa, "1 . 2 !\n", R"(
[  0]   1:1   cat=.number                                  1
[  1]   1:3   cat=.Test.oper                               .
[  2]   1:5   cat=.number                                  2
[  3]   1:7   cat=.Test.oper                               !

[0].Test.Exc.!                                    1 . 2 !
  [0].Test.Dot..                                  1 . 2
    [0].Test.Atom                                 1
    [1].Test.Atom                                 2
)");
    test::test_axe(*se, sa, "1 + 2 !\n", R"(
[  0]   1:1   cat=.number                                  1
[  1]   1:3   cat=.Test.oper                               +
[  2]   1:5   cat=.number                                  2
[  3]   1:7   cat=.Test.oper                               !

[0].Test.Add.+                                    1 + 2 !
  [0].Test.Atom                                   1
  [1].Test.Exc.!                                  2 !
    [0].Test.Atom                                 2
)");
    test::test_axe(*se, sa, "2 ! . 3\n", {none});
    test::test_axe(*se, sa, "2 . - 3\n", {none});
    test::test_axe(*se, sa, "2 $ !\n", R"(
[  0]   1:1   cat=.number                                  2
[  1]   1:3   cat=.Test.oper                               $
[  2]   1:5   cat=.Test.oper                               !

[0].Test.Exc.!                                    2 $ !
  [0].Test.Dol.$                                  2 $
    [0].Test.Atom                                 2
)");
    test::test_axe(*se, sa, "2 ! $\n", {none});
    test::test_axe(*se, sa, "+ ~ 2\n", R"(
[  0]   1:1   cat=.Test.oper                               +
[  1]   1:3   cat=.Test.oper                               ~
[  2]   1:5   cat=.number                                  2

[0].Test.Prf.+                                    + ~ 2
  [0].Test.Til.~                                  ~ 2
    [0].Test.Atom                                 2
)");
    test::test_axe(*se, sa, "~ + 2\n", {none});
    test::test_axe(*se, sa, "( ( 0 ) )\n", R"(
[  0]   1:1   cat=.literal                                 (
[  1]   1:3   cat=.literal                                 (
[  2]   1:5   cat=.number                                  0
[  3]   1:7   cat=.literal                                 )
[  4]   1:9   cat=.literal                                 )

[0].Test.Atom                                     ( ( 0 ) )
  [0].Test.Atom                                   ( 0 )
    [0].Test.Atom                                 0
)");
    test::test_axe(*se, sa, "1 * ( 2 + 3 ) * 4\n", R"(
[  0]   1:1   cat=.number                                  1
[  1]   1:3   cat=.Test.oper                               *
[  2]   1:5   cat=.literal                                 (
[  3]   1:7   cat=.number                                  2
[  4]   1:9   cat=.Test.oper                               +
[  5]   1:11  cat=.number                                  3
[  6]   1:13  cat=.literal                                 )
[  7]   1:15  cat=.Test.oper                               *
[  8]   1:17  cat=.number                                  4

[0].Test.Mul.*                                    1 * ... * 4
  [0].Test.Mul.*                                  1 * ... 3 )
    [0].Test.Atom                                 1
    [1].Test.Atom                                 ( 2 + 3 )
      [0].Test.Add.+                              2 + 3
        [0].Test.Atom                             2
        [1].Test.Atom                             3
  [1].Test.Atom                                   4
)");
    test::test_axe(*se, sa, "1 * ( 2 + 3 ) * 4\n", R"(
[  0]   1:1   cat=.number                                  1
[  1]   1:3   cat=.Test.oper                               *
[  2]   1:5   cat=.literal                                 (
[  3]   1:7   cat=.number                                  2
[  4]   1:9   cat=.Test.oper                               +
[  5]   1:11  cat=.number                                  3
[  6]   1:13  cat=.literal                                 )
[  7]   1:15  cat=.Test.oper                               *
[  8]   1:17  cat=.number                                  4

[0].Test.Mul.*                                    1 * ... * 4
  [0].Test.Mul.*                                  1 * ... 3 )
    [0].Test.Atom                                 1
    [1].Test.Atom                                 ( 2 + 3 )
      [0].Test.Add.+                              2 + 3
        [0].Test.Atom                             2
        [1].Test.Atom                             3
  [1].Test.Atom                                   4
)");
    test::test_axe(*se, sa, "a [ 0 ]\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               [
[  2]   1:5   cat=.number                                  0
[  3]   1:7   cat=.Test.oper                               ]

[0].Test.Sub.[                                    a [ 0 ]
  [0].Test.Atom                                   a
  [1].Test.Atom                                   0
)");
    test::test_axe(*se, sa, "a [ 0 ] [ 1 ]\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               [
[  2]   1:5   cat=.number                                  0
[  3]   1:7   cat=.Test.oper                               ]
[  4]   1:9   cat=.Test.oper                               [
[  5]   1:11  cat=.number                                  1
[  6]   1:13  cat=.Test.oper                               ]

[0].Test.Sub.[                                    a [ ... 1 ]
  [0].Test.Sub.[                                  a [ 0 ]
    [0].Test.Atom                                 a
    [1].Test.Atom                                 0
  [1].Test.Atom                                   1
)");
    test::test_axe(*se, sa, "a [ 0 ] . b [ 1 ]\n", {none});
    test::test_axe(*se, sa, "a [ 0 ] + b [ 1 ]\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               [
[  2]   1:5   cat=.number                                  0
[  3]   1:7   cat=.Test.oper                               ]
[  4]   1:9   cat=.Test.oper                               +
[  5]   1:11  cat=.identifier                              b
[  6]   1:13  cat=.Test.oper                               [
[  7]   1:15  cat=.number                                  1
[  8]   1:17  cat=.Test.oper                               ]

[0].Test.Add.+                                    a [ ... 1 ]
  [0].Test.Sub.[                                  a [ 0 ]
    [0].Test.Atom                                 a
    [1].Test.Atom                                 0
  [1].Test.Sub.[                                  b [ 1 ]
    [0].Test.Atom                                 b
    [1].Test.Atom                                 1
)");
    test::test_axe(*se, sa, "a ? b : c\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               ?
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               :
[  4]   1:9   cat=.identifier                              c

[0].Test.Ter.?                                    a ? b : c
  [0].Test.Atom                                   a
  [1].Test.Atom                                   b
  [2].Test.Atom                                   c
)");
    test::test_axe(*se, sa, "a ? b : c ? d : e\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               ?
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               :
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               ?
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               :
[  8]   1:17  cat=.identifier                              e

[0].Test.Ter.?                                    a ? ... : e
  [0].Test.Atom                                   a
  [1].Test.Atom                                   b
  [2].Test.Ter.?                                  c ? d : e
    [0].Test.Atom                                 c
    [1].Test.Atom                                 d
    [2].Test.Atom                                 e
)");
    test::test_axe(*se, sa, "a ? b ? c : d : e\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               ?
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               ?
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               :
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               :
[  8]   1:17  cat=.identifier                              e

[0].Test.Ter.?                                    a ? ... : e
  [0].Test.Atom                                   a
  [1].Test.Ter.?                                  b ? c : d
    [0].Test.Atom                                 b
    [1].Test.Atom                                 c
    [2].Test.Atom                                 d
  [2].Test.Atom                                   e
)");
    test::test_axe(*se, sa, "a = b ? c = d : e = f\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               =
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               ?
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               =
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               :
[  8]   1:17  cat=.identifier                              e
[  9]   1:19  cat=.Test.oper                               =
[ 10]   1:21  cat=.identifier                              f

[0].Test.Eqa.=                                    a = ... = f
  [0].Test.Atom                                   a
  [1].Test.Eqa.=                                  b ? ... = f
    [0].Test.Ter.?                                b ? ... : e
      [0].Test.Atom                               b
      [1].Test.Eqa.=                              c = d
        [0].Test.Atom                             c
        [1].Test.Atom                             d
      [2].Test.Atom                               e
    [1].Test.Atom                                 f
)");
    test::test_axe(*se, sa, "a + b ? c + d : e + f\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               +
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               ?
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               +
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               :
[  8]   1:17  cat=.identifier                              e
[  9]   1:19  cat=.Test.oper                               +
[ 10]   1:21  cat=.identifier                              f

[0].Test.Ter.?                                    a + ... + f
  [0].Test.Add.+                                  a + b
    [0].Test.Atom                                 a
    [1].Test.Atom                                 b
  [1].Test.Add.+                                  c + d
    [0].Test.Atom                                 c
    [1].Test.Atom                                 d
  [2].Test.Add.+                                  e + f
    [0].Test.Atom                                 e
    [1].Test.Atom                                 f
)");
  }

  TEST_CASE("seed-axe-advanced", "[seed-axe]")
  {
    syntax_farm_t sf;
    const unique_ptr_t<seed::interpreter_t> se = standard_seed_interpreter(sf.ptr());
    const string_view_t test_axe_str           = R"'(
language Test:
  ⊙ = axe Atom oper
    PrfHi   = rtl   prefix_nest '(' ')'
    Cat     = ltr   infix concat
    PrfLo   = rtl   prefix_nest '{' '}' prefix_nest -> Args '<:' ':>'
    Mul     = ltr   infix '*'
    Add     = ltr   infix_flat '+' infix '-'
    Assign  = rtl   infix_flat '=' infix '%'
  oper = '<:' | ':>' | operator.single | parenthesis
  Atom = identifier | number operator.single | '(' Test ')' | '<<' Test.PrfLo '>>'
  Args = string ( ',' string ) * | ε
  skip = skip.freeForm
)'";
    SILVA_REQUIRE(se->add_seed_text("test.seed", string_t{test_axe_str}));
    const auto& sa = se->axes.at(sf.name_id_of("Test"));
    CHECK(sa.concat_result.has_value());
    CHECK(sa.results.size() == 11);

    test::test_axe(*se, sa, "a\n", R"(
[  0]   1:1   cat=.identifier                              a

[0].Test.Atom                                     a
)");
    test::test_axe(*se, sa, "a y z\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.identifier                              y
[  2]   1:5   cat=.identifier                              z

[0].Test.Cat.concat                               a y z
  [0].Test.Cat.concat                             a y
    [0].Test.Atom                                 a
    [1].Test.Atom                                 y
  [1].Test.Atom                                   z
)");
    test::test_axe(*se, sa, "<: :> a\n", R"(
[  0]   1:1   cat=.Test.oper                               <:
[  1]   1:4   cat=.Test.oper                               :>
[  2]   1:7   cat=.identifier                              a

[0].Test.PrfLo.<:                                 <: :> a
  [0].Test.Args                                   
  [1].Test.Atom                                   a
)");
    test::test_axe(*se, sa, "<: 'foo' :> a\n", R"(
[  0]   1:1   cat=.Test.oper                               <:
[  1]   1:4   cat=.string                                  'foo'
[  2]   1:10  cat=.Test.oper                               :>
[  3]   1:13  cat=.identifier                              a

[0].Test.PrfLo.<:                                 <: 'foo' :> a
  [0].Test.Args                                   'foo'
  [1].Test.Atom                                   a
)");
    test::test_axe(*se, sa, "<: 'foo' , 'bar' , 'baz' :> a\n", R"(
[  0]   1:1   cat=.Test.oper                               <:
[  1]   1:4   cat=.string                                  'foo'
[  2]   1:10  cat=.literal                                 ,
[  3]   1:12  cat=.string                                  'bar'
[  4]   1:18  cat=.literal                                 ,
[  5]   1:20  cat=.string                                  'baz'
[  6]   1:26  cat=.Test.oper                               :>
[  7]   1:29  cat=.identifier                              a

[0].Test.PrfLo.<:                                 <: 'foo' ... :> a
  [0].Test.Args                                   'foo' , 'bar' , 'baz'
  [1].Test.Atom                                   a
)");
    test::test_axe(*se, sa, "a * <: 'foo' , 'bar' , 'baz' :> a\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               *
[  2]   1:5   cat=.Test.oper                               <:
[  3]   1:8   cat=.string                                  'foo'
[  4]   1:14  cat=.literal                                 ,
[  5]   1:16  cat=.string                                  'bar'
[  6]   1:22  cat=.literal                                 ,
[  7]   1:24  cat=.string                                  'baz'
[  8]   1:30  cat=.Test.oper                               :>
[  9]   1:33  cat=.identifier                              a

[0].Test.Mul.*                                    a * ... :> a
  [0].Test.Atom                                   a
  [1].Test.PrfLo.<:                               <: 'foo' ... :> a
    [0].Test.Args                                 'foo' , 'bar' , 'baz'
    [1].Test.Atom                                 a
)");
    test::test_axe(*se, sa, "{ b } a\n", R"(
[  0]   1:1   cat=.Test.oper                               {
[  1]   1:3   cat=.identifier                              b
[  2]   1:5   cat=.Test.oper                               }
[  3]   1:7   cat=.identifier                              a

[0].Test.PrfLo.{                                  { b } a
  [0].Test.Atom                                   b
  [1].Test.Atom                                   a
)");
    test::test_axe(*se, sa, "a { b } c\n", {none});
    test::test_axe(*se, sa, "a ( b ) c\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               (
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               )
[  4]   1:9   cat=.identifier                              c

[0].Test.Cat.concat                               a ( b ) c
  [0].Test.Atom                                   a
  [1].Test.PrfHi.(                                ( b ) c
    [0].Test.Atom                                 b
    [1].Test.Atom                                 c
)");
    test::test_axe(*se, sa, "a << { b } c >>\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.literal                                 <<
[  2]   1:6   cat=.Test.oper                               {
[  3]   1:8   cat=.identifier                              b
[  4]   1:10  cat=.Test.oper                               }
[  5]   1:12  cat=.identifier                              c
[  6]   1:14  cat=.literal                                 >>

[0].Test.Cat.concat                               a << ... c >>
  [0].Test.Atom                                   a
  [1].Test.Atom                                   << { ... c >>
    [0].Test.PrfLo.{                              { b } c
      [0].Test.Atom                               b
      [1].Test.Atom                               c
)");
    test::test_axe(*se, sa, "a << b * c >>\n", {none});
    test::test_axe(*se, sa, "<< a { b } >> c\n", {none});
    test::test_axe(*se, sa, "a 1 a z\n", {none});
    test::test_axe(*se, sa, "a 1 + z\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.number                                  1
[  2]   1:5   cat=.operator.single                         +
[  3]   1:7   cat=.identifier                              z

[0].Test.Cat.concat                               a 1 + z
  [0].Test.Cat.concat                             a 1 +
    [0].Test.Atom                                 a
    [1].Test.Atom                                 1 +
  [1].Test.Atom                                   z
)");
    test::test_axe(*se, sa, "a + b + c\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               +
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               +
[  4]   1:9   cat=.identifier                              c

[0].Test.Add.+                                    a + b + c
  [0].Test.Atom                                   a
  [1].Test.Atom                                   b
  [2].Test.Atom                                   c
)");
    test::test_axe(*se, sa, "a + b + c * d + e + f\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               +
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               +
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               *
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               +
[  8]   1:17  cat=.identifier                              e
[  9]   1:19  cat=.Test.oper                               +
[ 10]   1:21  cat=.identifier                              f

[0].Test.Add.+                                    a + ... + f
  [0].Test.Atom                                   a
  [1].Test.Atom                                   b
  [2].Test.Mul.*                                  c * d
    [0].Test.Atom                                 c
    [1].Test.Atom                                 d
  [3].Test.Atom                                   e
  [4].Test.Atom                                   f
)");
    test::test_axe(*se, sa, "a + b + c - d - e + f + g\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               +
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               +
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               -
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               -
[  8]   1:17  cat=.identifier                              e
[  9]   1:19  cat=.Test.oper                               +
[ 10]   1:21  cat=.identifier                              f
[ 11]   1:23  cat=.Test.oper                               +
[ 12]   1:25  cat=.identifier                              g

[0].Test.Add.+                                    a + ... + g
  [0].Test.Add.-                                  a + ... - e
    [0].Test.Add.-                                a + ... - d
      [0].Test.Add.+                              a + b + c
        [0].Test.Atom                             a
        [1].Test.Atom                             b
        [2].Test.Atom                             c
      [1].Test.Atom                               d
    [1].Test.Atom                                 e
  [1].Test.Atom                                   f
  [2].Test.Atom                                   g
)");
    test::test_axe(*se, sa, "a - b + c + d - e\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               -
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               +
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               +
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               -
[  8]   1:17  cat=.identifier                              e

[0].Test.Add.-                                    a - ... - e
  [0].Test.Add.+                                  a - ... + d
    [0].Test.Add.-                                a - b
      [0].Test.Atom                               a
      [1].Test.Atom                               b
    [1].Test.Atom                                 c
    [2].Test.Atom                                 d
  [1].Test.Atom                                   e
)");
    test::test_axe(*se, sa, "a + b + c - d + e + f\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               +
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               +
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               -
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               +
[  8]   1:17  cat=.identifier                              e
[  9]   1:19  cat=.Test.oper                               +
[ 10]   1:21  cat=.identifier                              f

[0].Test.Add.+                                    a + ... + f
  [0].Test.Add.-                                  a + ... - d
    [0].Test.Add.+                                a + b + c
      [0].Test.Atom                               a
      [1].Test.Atom                               b
      [2].Test.Atom                               c
    [1].Test.Atom                                 d
  [1].Test.Atom                                   e
  [2].Test.Atom                                   f
)");
    test::test_axe(*se, sa, "a % b = c = d % e\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               %
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               =
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               =
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               %
[  8]   1:17  cat=.identifier                              e

[0].Test.Assign.%                                 a % ... % e
  [0].Test.Atom                                   a
  [1].Test.Assign.=                               b = ... % e
    [0].Test.Atom                                 b
    [1].Test.Atom                                 c
    [2].Test.Assign.%                             d % e
      [0].Test.Atom                               d
      [1].Test.Atom                               e
)");
    test::test_axe(*se, sa, "a = b = c % d = e = f\n", R"(
[  0]   1:1   cat=.identifier                              a
[  1]   1:3   cat=.Test.oper                               =
[  2]   1:5   cat=.identifier                              b
[  3]   1:7   cat=.Test.oper                               =
[  4]   1:9   cat=.identifier                              c
[  5]   1:11  cat=.Test.oper                               %
[  6]   1:13  cat=.identifier                              d
[  7]   1:15  cat=.Test.oper                               =
[  8]   1:17  cat=.identifier                              e
[  9]   1:19  cat=.Test.oper                               =
[ 10]   1:21  cat=.identifier                              f

[0].Test.Assign.=                                 a = ... = f
  [0].Test.Atom                                   a
  [1].Test.Atom                                   b
  [2].Test.Assign.%                               c % ... = f
    [0].Test.Atom                                 c
    [1].Test.Assign.=                             d = e = f
      [0].Test.Atom                               d
      [1].Test.Atom                               e
      [2].Test.Atom                               f
)");
  }
}
