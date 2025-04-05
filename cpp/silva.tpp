#include "silva.hpp"

#include "syntax/parse_tree.hpp"
#include "syntax/seed_engine.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("operator-precedence", "")
{
  const string_view_t expr_seed_text = R"'(
    - Expr = Add
    - Add = Mult ( '+' Add ) *
    - Mult = Primary ( '*' Mult ) *
    - Primary = '(' Expr ')' | number
  )'";
  token_context_t tc;
  seed_engine_t se(tc.ptr());
  SILVA_EXPECT_REQUIRE(se.add_complete_file("expr.seed", expr_seed_text));

  const string_view_t expr_text = R"( 5 + 4 * 2 + 1 )";
  const auto expr_tt            = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", expr_text)));
  const auto expr_pt            = SILVA_EXPECT_REQUIRE(se.apply(expr_tt, tc.name_id_of("Expr")));

  const std::string_view expected_parse_tree = R"(
[0]Silva.Expr                                     5 + ... + 1
  [0]Silva.Add                                    5 + ... + 1
    [0]Silva.Mult                                 5
      [0]Silva.Primary                            5
    [1]Silva.Add                                  4 * 2 + 1
      [0]Silva.Mult                               4 * 2
        [0]Silva.Primary                          4
        [1]Silva.Mult                             2
          [0]Silva.Primary                        2
      [1]Silva.Add                                1
        [0]Silva.Mult                             1
          [0]Silva.Primary                        1
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
  token_context_t tc;
  seed_engine_t se(tc.ptr());
  SILVA_EXPECT_REQUIRE(se.add_complete_file("expr.seed", expr_seed_text));

  const string_view_t expr_text = R"(
    ( 5 + if a < 3 then b + 10 else c * 20 ) + 100
  )";
  const auto expr_tt            = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", expr_text)));
  const auto expr_pt            = SILVA_EXPECT_REQUIRE(se.apply(expr_tt, tc.name_id_of("Expr")));

  const std::string_view expected_parse_tree = R"(
[0]Silva.Expr.Add.+                               ( 5 ... + 100
  [0]Silva.Expr.Parens.(                          ( 5 ... 20 )
    [0]Silva.Expr.Add.+                           5 + ... * 20
      [0]Silva.Atom                               5
      [1]Silva.Atom                               if a ... * 20
        [0]Silva.Expr.Comp.<                      a < 3
          [0]Silva.Atom                           a
          [1]Silva.Atom                           3
        [1]Silva.Expr.Add.+                       b + 10
          [0]Silva.Atom                           b
          [1]Silva.Atom                           10
        [2]Silva.Expr.Mult.*                      c * 20
          [0]Silva.Atom                           c
          [1]Silva.Atom                           20
  [1]Silva.Atom                                   100
)";
  const string_t result{SILVA_EXPECT_REQUIRE(expr_pt->span().to_string())};
  CHECK(result == expected_parse_tree.substr(1));
}
