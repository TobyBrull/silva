#include "parse_root.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("exclamation mark", "[parse_root_t][seed]")
{
  const source_code_t frog_seed_source_code("frog.seed", R"'(
    - Seed = Rule*
    - Rule = RuleName "=" Expr
    - RuleName = identifier
    - Expr = Primary+
    - Primary = identifier "="!
  )'");

  const tokenization_t frog_seed_tokenization = tokenize(hybrid_ptr_const(&frog_seed_source_code));
  const parse_root_t pr = SILVA_TRY_REQUIRE(parse_root_t::create(&frog_seed_tokenization));

  const source_code_t frog_source_code("some.frog", R"'(
    SimpleFern = a b c
    LabeledItem = d e
    Label = f
    Item = g h i
  )'");

  const tokenization_t frog_tokens = tokenize(hybrid_ptr_const(&frog_source_code));
  const parse_tree_t frog_pt       = SILVA_TRY_REQUIRE(pr.apply(&frog_tokens));

  const std::string_view expected = R"(
[.]Seed,0                                         SimpleFern
  [0]Rule,0                                       SimpleFern
    [0]RuleName,0                                 SimpleFern
    [1]Expr,0                                     a
      [0]Primary,0                                a
      [1]Primary,0                                b
      [2]Primary,0                                c
  [1]Rule,0                                       LabeledItem
    [0]RuleName,0                                 LabeledItem
    [1]Expr,0                                     d
      [0]Primary,0                                d
      [1]Primary,0                                e
  [2]Rule,0                                       Label
    [0]RuleName,0                                 Label
    [1]Expr,0                                     f
      [0]Primary,0                                f
  [3]Rule,0                                       Item
    [0]RuleName,0                                 Item
    [1]Expr,0                                     g
      [0]Primary,0                                g
      [1]Primary,0                                h
      [2]Primary,0                                i
)";
  CHECK(parse_tree_to_string(frog_pt) == expected.substr(1));
}
