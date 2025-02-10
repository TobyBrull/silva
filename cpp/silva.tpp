#include "silva.hpp"

#include "parse_root.hpp"
#include "parse_tree.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("operator-precedence", "")
{
  const string_t op_prec_source_code = R"'(
    - Expr = Add
    - Add = Mult ("+" Add)*
    - Mult = Primary ("*" Mult)*
    - Primary,0 = "(" Expr ")"
    - Primary,1 = number
  )'";
  tokenization_t op_prec_tokens = SILVA_EXPECT_REQUIRE(token_context_make("", op_prec_source_code));
  const parse_root_t prec = SILVA_EXPECT_REQUIRE(parse_root_t::create(std::move(op_prec_tokens)));

  const string_t expr_source_code  = R"(
    5 + 4 * 2 + 1
  )";
  tokenization_t expr_tokenization = SILVA_EXPECT_REQUIRE(token_context_make("", expr_source_code));
  const auto pt                    = SILVA_EXPECT_REQUIRE(prec.apply(std::move(expr_tokenization)));

  const std::string_view expected_parse_tree = R"(
[0]Expr,0                                         5
  [0]Add,0                                        5
    [0]Mult,0                                     5
      [0]Primary,1                                5
    [1]Add,0                                      4
      [0]Mult,0                                   4
        [0]Primary,1                              4
        [1]Mult,0                                 2
          [0]Primary,1                            2
      [1]Add,0                                    1
        [0]Mult,0                                 1
          [0]Primary,1                            1
)";
  const string_t result{SILVA_EXPECT_REQUIRE(parse_tree_to_string(pt))};
  CHECK(result == expected_parse_tree.substr(1));
}
