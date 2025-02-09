#include "parse_root.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("exclamation-mark", "[parse_root_t][seed]")
{
  const string_t frog_seed = R"'(
    - Frog = Rule*
    - Rule = RuleName "=" Expr
    - RuleName = identifier
    - Expr = Primary+
    - Primary = identifier "="!
  )'";
  const tokenization_t frog_seed_tokens =
      SILVA_EXPECT_REQUIRE(token_context_make("frog.seed", string_t{frog_seed}));
  const parse_root_t pr = SILVA_EXPECT_REQUIRE(parse_root_t::create(&frog_seed_tokens));
  const string_view_t expected_seed_pt = R"(
[0]Seed,0                                         -
  [0]Rule,0                                       Frog
    [0]Nonterminal,0                              Frog
    [1]Derivation,0                               =
      [0]Atom,1                                   Rule
        [0]Primary,2                              Rule
          [0]Nonterminal,0                        Rule
        [1]Suffix,0                               *
  [1]Rule,0                                       Rule
    [0]Nonterminal,0                              Rule
    [1]Derivation,0                               =
      [0]Atom,1                                   RuleName
        [0]Primary,2                              RuleName
          [0]Nonterminal,0                        RuleName
      [1]Atom,1                                   "="
        [0]Primary,1                              "="
          [0]Terminal,1                           "="
      [2]Atom,1                                   Expr
        [0]Primary,2                              Expr
          [0]Nonterminal,0                        Expr
  [2]Rule,0                                       RuleName
    [0]Nonterminal,0                              RuleName
    [1]Derivation,0                               =
      [0]Atom,1                                   identifier
        [0]Primary,1                              identifier
          [0]Terminal,1                           identifier
  [3]Rule,0                                       Expr
    [0]Nonterminal,0                              Expr
    [1]Derivation,0                               =
      [0]Atom,1                                   Primary
        [0]Primary,2                              Primary
          [0]Nonterminal,0                        Primary
        [1]Suffix,0                               +
  [4]Rule,0                                       Primary
    [0]Nonterminal,0                              Primary
    [1]Derivation,0                               =
      [0]Atom,1                                   identifier
        [0]Primary,1                              identifier
          [0]Terminal,1                           identifier
      [1]Atom,1                                   "="
        [0]Primary,1                              "="
          [0]Terminal,1                           "="
        [1]Suffix,0                               !
)";
  CHECK(parse_tree_to_string(*pr.seed_parse_tree) == expected_seed_pt.substr(1));

  const string_t frog_source_code = R"'(
    SimpleFern = a b c
    LabeledItem = d e
    Label = f
    Item = g h i
  )'";
  const tokenization_t frog_tokens =
      SILVA_EXPECT_REQUIRE(token_context_make("some.frog", string_t{frog_source_code}));
  const parse_tree_t frog_pt = SILVA_EXPECT_REQUIRE(pr.apply(&frog_tokens));

  const string_view_t expected = R"(
[0]Frog,0                                         SimpleFern
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
