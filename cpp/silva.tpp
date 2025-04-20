#include "silva.hpp"

#include "syntax/parse_tree.hpp"
#include "syntax/seed_engine.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("operator-precedence", "")
  {
    const string_view_t expr_seed_text = R"'(
    - Expr = Add
    - Add = Mult ( '+' Add ) *
    - Mult = Primary ( '*' Mult ) *
    - Primary = '(' Expr ')' | number
  )'";
    syntax_ward_t sw;
    seed_engine_t se(sw.ptr());
    SILVA_EXPECT_REQUIRE(se.add_complete_file("expr.seed", expr_seed_text));

    const string_view_t expr_text = R"( 5 + 4 * 2 + 1 )";
    const auto expr_tt            = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "", expr_text));
    const auto expr_pt = SILVA_EXPECT_REQUIRE(se.apply(sw, expr_tt, sw.name_id_of("Expr")));

    const std::string_view expected_parse_tree = R"(
[0]_.Expr                                         5 + ... + 1
  [0]_.Add                                        5 + ... + 1
    [0]_.Mult                                     5
      [0]_.Primary                                5
    [1]_.Add                                      4 * 2 + 1
      [0]_.Mult                                   4 * 2
        [0]_.Primary                              4
        [1]_.Mult                                 2
          [0]_.Primary                            2
      [1]_.Add                                    1
        [0]_.Mult                                 1
          [0]_.Primary                            1
)";
    const string_t result{SILVA_EXPECT_REQUIRE(expr_pt->span().to_string())};
    CHECK(result == expected_parse_tree.substr(1));
  }

  TEST_CASE("parse-axe-recursion", "")
  {
    const string_view_t expr_seed_text = R"'(
    - Expr =/ Atom [
      - Parens  = nest  atom_nest '(' ')'
      - Mult    = ltr   infix '*'
      - Add     = ltr   infix '+'
      - Comp    = ltr   infix '<'
    ]
    - Atom = 'if' Expr 'then' Expr 'else' Expr | number | identifier
  )'";
    syntax_ward_t sw;
    seed_engine_t se(sw.ptr());
    SILVA_EXPECT_REQUIRE(se.add_complete_file("expr.seed", expr_seed_text));

    const string_view_t expr_text = R"(
    ( 5 + if a < 3 then b + 10 else c * 20 ) + 100
  )";
    const auto expr_tt            = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "", expr_text));
    const auto expr_pt = SILVA_EXPECT_REQUIRE(se.apply(sw, expr_tt, sw.name_id_of("Expr")));

    const std::string_view expected_parse_tree = R"(
[0]_.Expr.Add.+                                   ( 5 ... + 100
  [0]_.Expr.Parens.(                              ( 5 ... 20 )
    [0]_.Expr.Add.+                               5 + ... * 20
      [0]_.Atom                                   5
      [1]_.Atom                                   if a ... * 20
        [0]_.Expr.Comp.<                          a < 3
          [0]_.Atom                               a
          [1]_.Atom                               3
        [1]_.Expr.Add.+                           b + 10
          [0]_.Atom                               b
          [1]_.Atom                               10
        [2]_.Expr.Mult.*                          c * 20
          [0]_.Atom                               c
          [1]_.Atom                               20
  [1]_.Atom                                       100
)";
    const string_t result{SILVA_EXPECT_REQUIRE(expr_pt->span().to_string())};
    CHECK(result == expected_parse_tree.substr(1));
  }
}
