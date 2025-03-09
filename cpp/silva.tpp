#include "silva.hpp"

#include "parse_root.hpp"
#include "parse_tree.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("operator-precedence", "")
{
  token_context_t tc;
  const string_t op_prec_source_code = R"'(
    - Expr = Add
    - Add = Mult ( '+' Add ) *
    - Mult = Primary ( '*' Mult ) *
    - Primary = '(' Expr ')' | number
  )'";
  const auto op_prec_tt = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", op_prec_source_code)));
  const auto op_prec_pt = share(SILVA_EXPECT_REQUIRE(seed_parse(op_prec_tt)));
  const auto prec       = SILVA_EXPECT_REQUIRE(parse_root_t::create(op_prec_pt));

  const string_t expr_source_code = R"(
    5 + 4 * 2 + 1
  )";
  const auto expr_tt = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", expr_source_code)));
  const auto expr_pt = SILVA_EXPECT_REQUIRE(prec->apply(expr_tt));

  const std::string_view expected_parse_tree = R"(
[0]/Expr                                          5 + ... + 1
  [0]/Add                                         5 + ... + 1
    [0]/Mult                                      5
      [0]/Primary                                 5
    [1]/Add                                       4 * 2 + 1
      [0]/Mult                                    4 * 2
        [0]/Primary                               4
        [1]/Mult                                  2
          [0]/Primary                             2
      [1]/Add                                     1
        [0]/Mult                                  1
          [0]/Primary                             1
)";
  const string_t result{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*expr_pt))};
  CHECK(result == expected_parse_tree.substr(1));
}
