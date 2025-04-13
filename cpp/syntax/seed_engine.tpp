#include "seed_engine.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("not;but_then;keywords", "[seed_engine_t][seed]")
  {
    const string_view_t frog_seed = R"'(
    - Frog = [
      - X = Rule *
      - Rule = RuleName Expr
      - RuleName => keywords_of X.Keyword
      - Expr = Primary +
      - Primary = not keywords_of Keyword but_then identifier
      - Keyword = [
        - X = 'keyword1' | 'keyword2' | 'keyword3'
      ]
    ]
  )'";
    token_context_t tc;
    seed_engine_t se(tc.ptr());
    SILVA_EXPECT_REQUIRE(se.add_complete_file("frog.seed", frog_seed));
    const string_view_t expected_seed_pt = R"(
[0]Silva.Seed                                     - Frog ... ] ]
  [0]Silva.Seed.Rule                              Frog = ... ] ]
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
      [5]Silva.Seed.Rule                          Keyword = ... 'keyword3' ]
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
        SILVA_EXPECT_REQUIRE(se.seed_parse_trees.front()->span().to_string())};
    CHECK(seed_pt_str == expected_seed_pt.substr(1));

    const string_view_t frog_source_code = R"'(
    keyword1 a b c
    keyword2 d e
    keyword1 f
    keyword3 g h i
  )'";
    auto frog_tt = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", frog_source_code)));
    auto frog_pt = share(SILVA_EXPECT_REQUIRE(se.apply(frog_tt, tc.name_id_of("Frog"))));
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

  TEST_CASE("multiple-texts", "[seed_engine_t]")
  {
    const string_view_t text1_seed = R"'(
    - Foo = [
      - X = 'a' 'b' 'c' Silva.Bar ?
    ]
  )'";
    const string_view_t text2_seed = R"'(
    - Bar = [
      - Blub = 'u' 'v' 'w'
      - X = 'x' 'y' 'z' Silva.Foo ?
    ]
  )'";
    token_context_t tc;
    seed_engine_t se(tc.ptr());
    SILVA_EXPECT_REQUIRE(se.add_complete_file("text1.seed", text1_seed));
    SILVA_EXPECT_REQUIRE(se.add_complete_file("text2.seed", text2_seed));

    const string_view_t code     = R"'(
    a b c x y z a b c
  )'";
    auto tt                      = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", code)));
    auto pt                      = share(SILVA_EXPECT_REQUIRE(se.apply(tt, tc.name_id_of("Foo"))));
    const string_view_t expected = R"(
[0]Silva.Foo                                      a b ... b c
  [0]Silva.Bar                                    x y ... b c
    [0]Silva.Foo                                  a b c
)";
    const string_t result_str{SILVA_EXPECT_REQUIRE(pt->span().to_string())};
    CHECK(result_str == expected.substr(1));
  }
}
