#include "parse_root.hpp"
#include "seed.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("exclamation-mark", "[parse_root_t][seed]")
{
  token_context_t tc;
  const string_t frog_seed = R"'(
    - Frog = Rule *
    - Rule = RuleName '=' Expr
    - RuleName = identifier
    - Expr = Primary +
    - Primary = identifier '=' !
  )'";
  auto fs_tt               = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", frog_seed)));
  auto fs_pt               = share(SILVA_EXPECT_REQUIRE(seed_parse(fs_tt)));
  auto fs_pr               = share(SILVA_EXPECT_REQUIRE(parse_root_t::create(fs_pt)));
  const string_view_t expected_seed_pt = R"(
[0]Silva.Seed                                     - Frog ... '=' !
  [0]Silva.Rule                                   Frog = Rule *
    [0]Silva.Nonterminal                          Frog
    [1]Silva.Expr.Postfix.*                       Rule *
      [0]Silva.Nonterminal                        Rule
  [1]Silva.Rule                                   Rule = RuleName '=' Expr
    [0]Silva.Nonterminal                          Rule
    [1]Silva.Expr.Concat.concat                   RuleName '=' Expr
      [0]Silva.Nonterminal                        RuleName
      [1]Silva.Terminal                           '='
      [2]Silva.Nonterminal                        Expr
  [2]Silva.Rule                                   RuleName = identifier
    [0]Silva.Nonterminal                          RuleName
    [1]Silva.Terminal                             identifier
  [3]Silva.Rule                                   Expr = Primary +
    [0]Silva.Nonterminal                          Expr
    [1]Silva.Expr.Postfix.+                       Primary +
      [0]Silva.Nonterminal                        Primary
  [4]Silva.Rule                                   Primary = identifier '=' !
    [0]Silva.Nonterminal                          Primary
    [1]Silva.Expr.Concat.concat                   identifier '=' !
      [0]Silva.Terminal                           identifier
      [1]Silva.Expr.Postfix.!                     '=' !
        [0]Silva.Terminal                         '='
)";
  const string_t seed_pt_str{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*fs_pr->seed_parse_tree))};
  CHECK(seed_pt_str == expected_seed_pt.substr(1));

  const string_t frog_source_code = R"'(
    SimpleFern = a b c
    LabeledItem = d e
    Label = f
    Item = g h i
  )'";
  auto frog_tt = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", frog_source_code)));
  auto frog_pt = share(SILVA_EXPECT_REQUIRE(fs_pr->apply(frog_tt)));
  const string_view_t expected = R"(
[0]Silva.Frog                                     SimpleFern = ... h i
  [0]Silva.Rule                                   SimpleFern = a b c
    [0]Silva.RuleName                             SimpleFern
    [1]Silva.Expr                                 a b c
      [0]Silva.Primary                            a
      [1]Silva.Primary                            b
      [2]Silva.Primary                            c
  [1]Silva.Rule                                   LabeledItem = d e
    [0]Silva.RuleName                             LabeledItem
    [1]Silva.Expr                                 d e
      [0]Silva.Primary                            d
      [1]Silva.Primary                            e
  [2]Silva.Rule                                   Label = f
    [0]Silva.RuleName                             Label
    [1]Silva.Expr                                 f
      [0]Silva.Primary                            f
  [3]Silva.Rule                                   Item = g h i
    [0]Silva.RuleName                             Item
    [1]Silva.Expr                                 g h i
      [0]Silva.Primary                            g
      [1]Silva.Primary                            h
      [2]Silva.Primary                            i
)";
  const string_t frog_pt_str{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*frog_pt))};
  CHECK(frog_pt_str == expected.substr(1));
}
