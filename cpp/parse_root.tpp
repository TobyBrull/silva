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
[0]/Seed                                          - Frog ... '=' !
  [0]/Rule                                        Frog = Rule *
    [0]/Nonterminal                               Frog
    [1]/Expr/Postfix/*                            Rule *
      [0]/Nonterminal                             Rule
  [1]/Rule                                        Rule = RuleName '=' Expr
    [0]/Nonterminal                               Rule
    [1]/Expr/Concat/                              RuleName '=' Expr
      [0]/Nonterminal                             RuleName
      [1]/Terminal                                '='
      [2]/Nonterminal                             Expr
  [2]/Rule                                        RuleName = identifier
    [0]/Nonterminal                               RuleName
    [1]/Terminal                                  identifier
  [3]/Rule                                        Expr = Primary +
    [0]/Nonterminal                               Expr
    [1]/Expr/Postfix/+                            Primary +
      [0]/Nonterminal                             Primary
  [4]/Rule                                        Primary = identifier '=' !
    [0]/Nonterminal                               Primary
    [1]/Expr/Concat/                              identifier '=' !
      [0]/Terminal                                identifier
      [1]/Expr/Postfix/!                          '=' !
        [0]/Terminal                              '='
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
[0]/Frog                                          SimpleFern = ... h i
  [0]/Rule                                        SimpleFern = a b c
    [0]/RuleName                                  SimpleFern
    [1]/Expr                                      a b c
      [0]/Primary                                 a
      [1]/Primary                                 b
      [2]/Primary                                 c
  [1]/Rule                                        LabeledItem = d e
    [0]/RuleName                                  LabeledItem
    [1]/Expr                                      d e
      [0]/Primary                                 d
      [1]/Primary                                 e
  [2]/Rule                                        Label = f
    [0]/RuleName                                  Label
    [1]/Expr                                      f
      [0]/Primary                                 f
  [3]/Rule                                        Item = g h i
    [0]/RuleName                                  Item
    [1]/Expr                                      g h i
      [0]/Primary                                 g
      [1]/Primary                                 h
      [2]/Primary                                 i
)";
  const string_t frog_pt_str{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*frog_pt))};
  CHECK(frog_pt_str == expected.substr(1));
}
