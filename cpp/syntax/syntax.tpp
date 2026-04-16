#include "syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("operator-precedence", "")
  {
    const string_view_t expr_seed_text = R"'(
tokenizer Expr:
  ignore WHITESPACE
  ignore COMMENT
  ignore INDENT
  ignore DEDENT
  ignore NEWLINE
  number = NUMBER
  string = STRING
  operator = PARENTHESIS
  operator = ::: OPERATOR

language Expr:
  ⊙ = Add
Add = Mult ( '+' Add ) *
Mult = Primary ( '*' Mult ) *
Primary = '(' Expr ')' | number
)'";
    syntax_farm_t sf;
    seed::interpreter_t si(sf.ptr());
    SILVA_REQUIRE(si.add_seed_text("expr.seed", string_t{expr_seed_text}));

    const string_t expr_text = " 5 + 4 * 2 + 1\n";

    const auto expr_pt = SILVA_REQUIRE(si.apply_text("", expr_text, sf.name_id_of("Expr")));

    const std::string_view expected_parse_tree = R"(
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
tokenizer Expr:
  ignore WHITESPACE
  ignore COMMENT
  ignore INDENT
  ignore DEDENT
  ignore NEWLINE
  number = NUMBER
  string = STRING
  identifier = IDENTIFIER
  operator = PARENTHESIS
  operator = ::: OPERATOR

language Expr:
  ⊙ = axe Atom
    Parens  = nest  atom_nest '(' ')'
    Mult    = ltr   infix '*'
    Add     = ltr   infix '+'
    Comp    = ltr   infix '<'
Atom = 'if' Expr 'then' Expr 'else' Expr | number | identifier
)'";
    syntax_farm_t sf;
    seed::interpreter_t si(sf.ptr());
    SILVA_REQUIRE(si.add_seed_text("expr.seed", string_t{expr_seed_text}));

    const string_t expr_text = R"(
    ( 5 + if a < 3 then b + 10 else c * 20 ) + 100
)";

    const auto expr_pt = SILVA_REQUIRE(si.apply_text("", expr_text, sf.name_id_of("Expr")));

    const std::string_view expected_parse_tree = R"(
[0].Expr.Add.+                                    ( 5 ... + 100
  [0].Expr.Parens.(                               ( 5 ... 20 )
    [0].Expr.Add.+                                5 + ... * 20
      [0].Atom                                    5
      [1].Atom                                    if a ... * 20
        [0].Expr.Comp.<                           a < 3
          [0].Atom                                a
          [1].Atom                                3
        [1].Expr.Add.+                            b + 10
          [0].Atom                                b
          [1].Atom                                10
        [2].Expr.Mult.*                           c * 20
          [0].Atom                                c
          [1].Atom                                20
  [1].Atom                                        100
)";
    const string_t result{SILVA_REQUIRE(expr_pt->span().to_string())};
    CHECK(result == expected_parse_tree.substr(1));
  }
}
