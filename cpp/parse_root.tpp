#include "parse_root.hpp"
#include "seed.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("exclamation-mark", "[parse_root_t][seed]")
{
  token_context_t tc;
  const string_t frog_seed = R"'(
    - Frog = Rule*
    - Rule = RuleName "=" Expr
    - RuleName = identifier
    - Expr = Primary+
    - Primary = identifier "="!
  )'";
  auto fs_tt               = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", frog_seed)));
  auto fs_pt               = share(SILVA_EXPECT_REQUIRE(seed_parse(fs_tt)));
  auto fs_pr               = share(SILVA_EXPECT_REQUIRE(parse_root_t::create(fs_pt)));
  const string_view_t expected_seed_pt = R"(
[0]`Seed`0                                        - Frog = ...
  [0]`Rule`0                                      Frog = Rule ...
    [0]`Nonterminal`0                             Frog
    [1]`Derivation`0                              = Rule *
      [0]`Atom`1                                  Rule *
        [0]`Primary`2                             Rule
          [0]`Nonterminal`0                       Rule
        [1]`Suffix`0                              *
  [1]`Rule`0                                      Rule = RuleName ...
    [0]`Nonterminal`0                             Rule
    [1]`Derivation`0                              = RuleName "=" ...
      [0]`Atom`1                                  RuleName
        [0]`Primary`2                             RuleName
          [0]`Nonterminal`0                       RuleName
      [1]`Atom`1                                  "="
        [0]`Primary`1                             "="
          [0]`Terminal`1                          "="
      [2]`Atom`1                                  Expr
        [0]`Primary`2                             Expr
          [0]`Nonterminal`0                       Expr
  [2]`Rule`0                                      RuleName = identifier
    [0]`Nonterminal`0                             RuleName
    [1]`Derivation`0                              = identifier
      [0]`Atom`1                                  identifier
        [0]`Primary`1                             identifier
          [0]`Terminal`1                          identifier
  [3]`Rule`0                                      Expr = Primary ...
    [0]`Nonterminal`0                             Expr
    [1]`Derivation`0                              = Primary +
      [0]`Atom`1                                  Primary +
        [0]`Primary`2                             Primary
          [0]`Nonterminal`0                       Primary
        [1]`Suffix`0                              +
  [4]`Rule`0                                      Primary = identifier ...
    [0]`Nonterminal`0                             Primary
    [1]`Derivation`0                              = identifier "=" ...
      [0]`Atom`1                                  identifier
        [0]`Primary`1                             identifier
          [0]`Terminal`1                          identifier
      [1]`Atom`1                                  "=" !
        [0]`Primary`1                             "="
          [0]`Terminal`1                          "="
        [1]`Suffix`0                              !
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
[0]`Frog`0                                        SimpleFern = a ...
  [0]`Rule`0                                      SimpleFern = a ...
    [0]`RuleName`0                                SimpleFern
    [1]`Expr`0                                    a b c
      [0]`Primary`0                               a
      [1]`Primary`0                               b
      [2]`Primary`0                               c
  [1]`Rule`0                                      LabeledItem = d ...
    [0]`RuleName`0                                LabeledItem
    [1]`Expr`0                                    d e
      [0]`Primary`0                               d
      [1]`Primary`0                               e
  [2]`Rule`0                                      Label = f
    [0]`RuleName`0                                Label
    [1]`Expr`0                                    f
      [0]`Primary`0                               f
  [3]`Rule`0                                      Item = g ...
    [0]`RuleName`0                                Item
    [1]`Expr`0                                    g h i
      [0]`Primary`0                               g
      [1]`Primary`0                               h
      [2]`Primary`0                               i
)";
  const string_t frog_pt_str{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*frog_pt))};
  CHECK(frog_pt_str == expected.substr(1));
}
