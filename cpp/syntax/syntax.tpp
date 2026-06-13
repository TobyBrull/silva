#include "syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("operator-precedence", "")
  {
    const string_view_t expr_seed_text = R"'(
language Expr:
  ⊙ = Add
  skip = ( SPACE | LINEFEED | COMMENT | WHITESPACE | INDENT | DEDENT | NEWLINE ) *
Add = Mult ( '+' Add ) *
Mult = Primary ( '*' Mult ) *
Primary = '(' Expr ')' | number
number = DIGIT +
)'";
    syntax_farm_t sf;
    seed::interpreter_t si(sf.ptr());
    SILVA_REQUIRE(si.add_seed_text("expr.seed", string_t{expr_seed_text}));

    const string_t expr_text = " 5 + 4 * 2 + 1\n";

    const auto expr_pt = SILVA_REQUIRE(si.apply_text("", expr_text, sf.name_id_of("Expr")));

    const std::string_view expected_parse_tree = R"(
[  0]   1:2   cat=.number                                  5
[  1]   1:4   cat=.literal                                 +
[  2]   1:6   cat=.number                                  4
[  3]   1:8   cat=.literal                                 *
[  4]   1:10  cat=.number                                  2
[  5]   1:12  cat=.literal                                 +
[  6]   1:14  cat=.number                                  1

[0].Expr                                          5 + ... + 1
  [0].Add                                         5 + ... + 1
    [0].Mult                                      5
      [0].Primary                                 5
    [1].Add                                       4 * 2 + 1
      [0].Mult                                    4 * 2
        [0].Primary                               4
        [1].Mult                                  2
          [0].Primary                             2
      [1].Add                                     1
        [0].Mult                                  1
          [0].Primary                             1
)";
    const string_t result{SILVA_REQUIRE(expr_pt->span().to_string())};
    CHECK(result == expected_parse_tree.substr(1));
  }

  TEST_CASE("seed-axe-recursion", "")
  {
    const string_view_t expr_seed_text = R"'(
language Expr:
  ⊙ = axe Atom operator
    Mult    = ltr   infix '*'
    Add     = ltr   infix '+'
    Comp    = ltr   infix '<'
  Atom = 'if' Expr 'then' Expr 'else' Expr | number | identifier | '(' Expr ')'
  skip = ( SPACE | LINEFEED | COMMENT | WHITESPACE | INDENT | DEDENT | NEWLINE ) *
  number = DIGIT +
  identifier = ID_START ID_CONTINUE *
  operator = OPERATOR
)'";
    syntax_farm_t sf;
    seed::interpreter_t si(sf.ptr());
    SILVA_REQUIRE(si.add_seed_text("expr.seed", string_t{expr_seed_text}));

    const string_t expr_text = R"(
    ( 5 + if a < 3 then b + 10 else c * 20 ) + 100
)";

    const auto expr_pt = SILVA_REQUIRE(si.apply_text("", expr_text, sf.name_id_of("Expr")));

    const std::string_view expected_parse_tree = R"(
[  0]   2:5   cat=.literal                                 (
[  1]   2:7   cat=.Expr.number                             5
[  2]   2:9   cat=.Expr.operator                           +
[  3]   2:11  cat=.literal                                 if
[  4]   2:14  cat=.Expr.identifier                         a
[  5]   2:16  cat=.Expr.operator                           <
[  6]   2:18  cat=.Expr.number                             3
[  7]   2:20  cat=.literal                                 then
[  8]   2:25  cat=.Expr.identifier                         b
[  9]   2:27  cat=.Expr.operator                           +
[ 10]   2:29  cat=.Expr.number                             10
[ 11]   2:32  cat=.literal                                 else
[ 12]   2:37  cat=.Expr.identifier                         c
[ 13]   2:39  cat=.Expr.operator                           *
[ 14]   2:41  cat=.Expr.number                             20
[ 15]   2:44  cat=.literal                                 )
[ 16]   2:46  cat=.Expr.operator                           +
[ 17]   2:48  cat=.Expr.number                             100

[0].Expr.Add.+                                    ( 5 ... + 100
  [0].Expr.Atom                                   ( 5 ... 20 )
    [0].Expr.Add.+                                5 + ... * 20
      [0].Expr.Atom                               5
      [1].Expr.Atom                               if a ... * 20
        [0].Expr.Comp.<                           a < 3
          [0].Expr.Atom                           a
          [1].Expr.Atom                           3
        [1].Expr.Add.+                            b + 10
          [0].Expr.Atom                           b
          [1].Expr.Atom                           10
        [2].Expr.Mult.*                           c * 20
          [0].Expr.Atom                           c
          [1].Expr.Atom                           20
  [1].Expr.Atom                                   100
)";
    const string_t result{SILVA_REQUIRE(expr_pt->span().to_string())};
    CHECK(result == expected_parse_tree.substr(1));
  }
}
