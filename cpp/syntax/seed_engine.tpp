#include "seed_engine.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("exclamation-mark", "[seed_engine_t][seed]")
{
  token_context_t tc;
  const string_t frog_seed = R"'(
    - Frog = Rule *
    - Rule = RuleName '=' Expr
    - RuleName => identifier
    - Expr = Primary +
    - Primary = identifier '=' !
  )'";
  auto fs_tt               = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", frog_seed)));
  auto fs_pt               = share(SILVA_EXPECT_REQUIRE(seed_parse(fs_tt)));
  auto fs_pr               = share(SILVA_EXPECT_REQUIRE(seed_engine_t::create(fs_pt)));
  const string_view_t expected_seed_pt = R"(
[0]Silva.Seed                                     - Frog ... '=' !
  [0]Silva.Seed.Rule                              Frog = Rule *
    [0]Silva.Seed.Nonterminal.Base                Frog
    [1]Silva.Seed.ExprOrAlias                     = Rule *
      [0]Silva.Seed.Expr.Postfix.*                Rule *
        [0]Silva.Seed.Nonterminal                 Rule
          [0]Silva.Seed.Nonterminal.Base          Rule
  [1]Silva.Seed.Rule                              Rule = RuleName '=' Expr
    [0]Silva.Seed.Nonterminal.Base                Rule
    [1]Silva.Seed.ExprOrAlias                     = RuleName '=' Expr
      [0]Silva.Seed.Expr.Concat.concat            RuleName '=' Expr
        [0]Silva.Seed.Nonterminal                 RuleName
          [0]Silva.Seed.Nonterminal.Base          RuleName
        [1]Silva.Seed.Terminal                    '='
        [2]Silva.Seed.Nonterminal                 Expr
          [0]Silva.Seed.Nonterminal.Base          Expr
  [2]Silva.Seed.Rule                              RuleName => identifier
    [0]Silva.Seed.Nonterminal.Base                RuleName
    [1]Silva.Seed.ExprOrAlias                     => identifier
      [0]Silva.Seed.Terminal                      identifier
  [3]Silva.Seed.Rule                              Expr = Primary +
    [0]Silva.Seed.Nonterminal.Base                Expr
    [1]Silva.Seed.ExprOrAlias                     = Primary +
      [0]Silva.Seed.Expr.Postfix.+                Primary +
        [0]Silva.Seed.Nonterminal                 Primary
          [0]Silva.Seed.Nonterminal.Base          Primary
  [4]Silva.Seed.Rule                              Primary = identifier '=' !
    [0]Silva.Seed.Nonterminal.Base                Primary
    [1]Silva.Seed.ExprOrAlias                     = identifier '=' !
      [0]Silva.Seed.Expr.Concat.concat            identifier '=' !
        [0]Silva.Seed.Terminal                    identifier
        [1]Silva.Seed.Expr.Postfix.!              '=' !
          [0]Silva.Seed.Terminal                  '='
)";
  const string_t seed_pt_str{
      SILVA_EXPECT_REQUIRE(parse_tree_to_string(*fs_pr->seed_parse_trees.front()))};
  CHECK(seed_pt_str == expected_seed_pt.substr(1));

  const string_t frog_source_code = R"'(
    SimpleFern = a b c
    LabeledItem = d e
    Label = f
    Item = g h i
  )'";
  auto frog_tt = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", frog_source_code)));
  auto frog_pt = share(SILVA_EXPECT_REQUIRE(fs_pr->apply(frog_tt, tc.full_name_id_of("Frog"))));
  const string_view_t expected = R"(
[0]Silva.Frog                                     SimpleFern = ... h i
  [0]Silva.Rule                                   SimpleFern = a b c
    [0]Silva.Expr                                 a b c
      [0]Silva.Primary                            a
      [1]Silva.Primary                            b
      [2]Silva.Primary                            c
  [1]Silva.Rule                                   LabeledItem = d e
    [0]Silva.Expr                                 d e
      [0]Silva.Primary                            d
      [1]Silva.Primary                            e
  [2]Silva.Rule                                   Label = f
    [0]Silva.Expr                                 f
      [0]Silva.Primary                            f
  [3]Silva.Rule                                   Item = g h i
    [0]Silva.Expr                                 g h i
      [0]Silva.Primary                            g
      [1]Silva.Primary                            h
      [2]Silva.Primary                            i
)";
  const string_t frog_pt_str{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*frog_pt))};
  CHECK(frog_pt_str == expected.substr(1));
}
