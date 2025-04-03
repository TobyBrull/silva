#include "seed_engine.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("not;but_then;keywords", "[seed_engine_t][seed]")
{
  syntax_context_t tc;
  const string_t frog_seed = R"'(
    - Frog [
      - X = Rule *
      - Rule = RuleName Expr
      - RuleName => keywords_of X.Keyword
      - Expr = Primary +
      - Primary = not keywords_of Keyword but_then identifier
      - Keyword [
        - X = 'keyword1' | 'keyword2' | 'keyword3'
      ]
    ]
  )'";
  auto fs_tt               = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", frog_seed)));
  auto fs_pt               = share(SILVA_EXPECT_REQUIRE(seed_parse(fs_tt)));
  auto fs_pr               = share(SILVA_EXPECT_REQUIRE(seed_engine_t::create(fs_pt)));
  const string_view_t expected_seed_pt = R"(
[0]Silva.Seed                                     - Frog ... ] ]
  [0]Silva.Seed.Rule                              Frog [ ... ] ]
    [0]Silva.Seed.Nonterminal.Base                Frog
    [1]Silva.Seed                                 - X ... 'keyword3' ]
      [0]Silva.Seed.Rule                          X = Rule *
        [0]Silva.Seed.Nonterminal.Base            X
        [1]Silva.Seed.ExprOrAlias                 = Rule *
          [0]Silva.Seed.Expr.Postfix.*            Rule *
            [0]Silva.Seed.Nonterminal             Rule
              [0]Silva.Seed.Nonterminal.Base      Rule
      [1]Silva.Seed.Rule                          Rule = RuleName Expr
        [0]Silva.Seed.Nonterminal.Base            Rule
        [1]Silva.Seed.ExprOrAlias                 = RuleName Expr
          [0]Silva.Seed.Expr.Concat.concat        RuleName Expr
            [0]Silva.Seed.Nonterminal             RuleName
              [0]Silva.Seed.Nonterminal.Base      RuleName
            [1]Silva.Seed.Nonterminal             Expr
              [0]Silva.Seed.Nonterminal.Base      Expr
      [2]Silva.Seed.Rule                          RuleName => ... . Keyword
        [0]Silva.Seed.Nonterminal.Base            RuleName
        [1]Silva.Seed.ExprOrAlias                 => keywords_of X . Keyword
          [0]Silva.Seed.Terminal                  keywords_of X . Keyword
            [0]Silva.Seed.Nonterminal             X . Keyword
              [0]Silva.Seed.Nonterminal.Base      X
              [1]Silva.Seed.Nonterminal.Base      Keyword
      [3]Silva.Seed.Rule                          Expr = Primary +
        [0]Silva.Seed.Nonterminal.Base            Expr
        [1]Silva.Seed.ExprOrAlias                 = Primary +
          [0]Silva.Seed.Expr.Postfix.+            Primary +
            [0]Silva.Seed.Nonterminal             Primary
              [0]Silva.Seed.Nonterminal.Base      Primary
      [4]Silva.Seed.Rule                          Primary = ... but_then identifier
        [0]Silva.Seed.Nonterminal.Base            Primary
        [1]Silva.Seed.ExprOrAlias                 = not ... but_then identifier
          [0]Silva.Seed.Expr.And.but_then         not keywords_of Keyword but_then identifier
            [0]Silva.Seed.Expr.Prefix.not         not keywords_of Keyword
              [0]Silva.Seed.Terminal              keywords_of Keyword
                [0]Silva.Seed.Nonterminal         Keyword
                  [0]Silva.Seed.Nonterminal.Base  Keyword
            [1]Silva.Seed.Terminal                identifier
      [5]Silva.Seed.Rule                          Keyword [ ... 'keyword3' ]
        [0]Silva.Seed.Nonterminal.Base            Keyword
        [1]Silva.Seed                             - X ... | 'keyword3'
          [0]Silva.Seed.Rule                      X = ... | 'keyword3'
            [0]Silva.Seed.Nonterminal.Base        X
            [1]Silva.Seed.ExprOrAlias             = 'keyword1' ... | 'keyword3'
              [0]Silva.Seed.Expr.Or.|             'keyword1' | 'keyword2' | 'keyword3'
                [0]Silva.Seed.Terminal            'keyword1'
                [1]Silva.Seed.Terminal            'keyword2'
                [2]Silva.Seed.Terminal            'keyword3'
)";
  const string_t seed_pt_str{
      SILVA_EXPECT_REQUIRE(fs_pr->seed_parse_trees.front()->span().to_string())};
  CHECK(seed_pt_str == expected_seed_pt.substr(1));

  const string_t frog_source_code = R"'(
    keyword1 a b c
    keyword2 d e
    keyword1 f
    keyword3 g h i
  )'";
  auto frog_tt = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", frog_source_code)));
  auto frog_pt = share(SILVA_EXPECT_REQUIRE(fs_pr->apply(frog_tt, tc.name_id_of("Frog"))));
  const string_view_t expected = R"(
[0]Silva.Frog                                     keyword1 a ... h i
  [0]Silva.Frog.Rule                              keyword1 a b c
    [0]Silva.Frog.Expr                            a b c
      [0]Silva.Frog.Primary                       a
      [1]Silva.Frog.Primary                       b
      [2]Silva.Frog.Primary                       c
  [1]Silva.Frog.Rule                              keyword2 d e
    [0]Silva.Frog.Expr                            d e
      [0]Silva.Frog.Primary                       d
      [1]Silva.Frog.Primary                       e
  [2]Silva.Frog.Rule                              keyword1 f
    [0]Silva.Frog.Expr                            f
      [0]Silva.Frog.Primary                       f
  [3]Silva.Frog.Rule                              keyword3 g h i
    [0]Silva.Frog.Expr                            g h i
      [0]Silva.Frog.Primary                       g
      [1]Silva.Frog.Primary                       h
      [2]Silva.Frog.Primary                       i
)";
  const string_t frog_pt_str{SILVA_EXPECT_REQUIRE(frog_pt->span().to_string())};
  CHECK(frog_pt_str == expected.substr(1));
}
