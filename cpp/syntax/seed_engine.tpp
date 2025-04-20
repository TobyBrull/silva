#include "seed_engine.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("not;but_then;keywords", "[seed_engine_t][seed]")
  {
    const string_view_t frog_seed = R"'(
    - Frog = [
      - x = Rule *
      - Rule = RuleName Expr
      - RuleName => keywords_of x.Keyword
      - Expr = Primary +
      - Primary = not keywords_of Keyword but_then identifier
      - Keyword = [
        - x = 'keyword1' | 'keyword2' | 'keyword3'
      ]
    ]
  )'";
    syntax_ward_t sw;
    seed_engine_t se(sw.ptr());
    auto ptp = SILVA_EXPECT_REQUIRE(se.add_complete_file("frog.seed", frog_seed));
    const string_view_t expected_seed_pt = R"(
[0]_.Seed                                         - Frog ... ] ]
  [0]_.Seed.Rule                                  Frog = ... ] ]
    [0]_.Seed.Nonterminal.Base                    Frog
    [1]_.Seed                                     - x ... 'keyword3' ]
      [0]_.Seed.Rule                              x = Rule *
        [0]_.Seed.Nonterminal.Base                x
        [1]_.Seed.ExprOrAlias                     = Rule *
          [0]_.Seed.Expr.Postfix.*                Rule *
            [0]_.Seed.Nonterminal                 Rule
              [0]_.Seed.Nonterminal.Base          Rule
      [1]_.Seed.Rule                              Rule = RuleName Expr
        [0]_.Seed.Nonterminal.Base                Rule
        [1]_.Seed.ExprOrAlias                     = RuleName Expr
          [0]_.Seed.Expr.Concat.concat            RuleName Expr
            [0]_.Seed.Nonterminal                 RuleName
              [0]_.Seed.Nonterminal.Base          RuleName
            [1]_.Seed.Nonterminal                 Expr
              [0]_.Seed.Nonterminal.Base          Expr
      [2]_.Seed.Rule                              RuleName => ... . Keyword
        [0]_.Seed.Nonterminal.Base                RuleName
        [1]_.Seed.ExprOrAlias                     => keywords_of x . Keyword
          [0]_.Seed.Terminal                      keywords_of x . Keyword
            [0]_.Seed.Nonterminal                 x . Keyword
              [0]_.Seed.Nonterminal.Base          x
              [1]_.Seed.Nonterminal.Base          Keyword
      [3]_.Seed.Rule                              Expr = Primary +
        [0]_.Seed.Nonterminal.Base                Expr
        [1]_.Seed.ExprOrAlias                     = Primary +
          [0]_.Seed.Expr.Postfix.+                Primary +
            [0]_.Seed.Nonterminal                 Primary
              [0]_.Seed.Nonterminal.Base          Primary
      [4]_.Seed.Rule                              Primary = ... but_then identifier
        [0]_.Seed.Nonterminal.Base                Primary
        [1]_.Seed.ExprOrAlias                     = not ... but_then identifier
          [0]_.Seed.Expr.And.but_then             not keywords_of Keyword but_then identifier
            [0]_.Seed.Expr.Prefix.not             not keywords_of Keyword
              [0]_.Seed.Terminal                  keywords_of Keyword
                [0]_.Seed.Nonterminal             Keyword
                  [0]_.Seed.Nonterminal.Base      Keyword
            [1]_.Seed.Terminal                    identifier
      [5]_.Seed.Rule                              Keyword = ... 'keyword3' ]
        [0]_.Seed.Nonterminal.Base                Keyword
        [1]_.Seed                                 - x ... | 'keyword3'
          [0]_.Seed.Rule                          x = ... | 'keyword3'
            [0]_.Seed.Nonterminal.Base            x
            [1]_.Seed.ExprOrAlias                 = 'keyword1' ... | 'keyword3'
              [0]_.Seed.Expr.Or.|                 'keyword1' | 'keyword2' | 'keyword3'
                [0]_.Seed.Terminal                'keyword1'
                [1]_.Seed.Terminal                'keyword2'
                [2]_.Seed.Terminal                'keyword3'
)";
    const string_t seed_pt_str{SILVA_EXPECT_REQUIRE(ptp->span().to_string())};
    CHECK(seed_pt_str == expected_seed_pt.substr(1));

    const string_view_t frog_source_code = R"'(
    keyword1 a b c
    keyword2 d e
    keyword1 f
    keyword3 g h i
  )'";
    auto frog_tt = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "", frog_source_code));
    auto frog_pt = SILVA_EXPECT_REQUIRE(se.apply(sw, frog_tt, sw.name_id_of("Frog")));
    const string_view_t expected = R"(
[0]_.Frog                                         keyword1 a ... h i
  [0]_.Frog.Rule                                  keyword1 a b c
    [0]_.Frog.Expr                                a b c
      [0]_.Frog.Primary                           a
      [1]_.Frog.Primary                           b
      [2]_.Frog.Primary                           c
  [1]_.Frog.Rule                                  keyword2 d e
    [0]_.Frog.Expr                                d e
      [0]_.Frog.Primary                           d
      [1]_.Frog.Primary                           e
  [2]_.Frog.Rule                                  keyword1 f
    [0]_.Frog.Expr                                f
      [0]_.Frog.Primary                           f
  [3]_.Frog.Rule                                  keyword3 g h i
    [0]_.Frog.Expr                                g h i
      [0]_.Frog.Primary                           g
      [1]_.Frog.Primary                           h
      [2]_.Frog.Primary                           i
)";
    const string_t frog_pt_str{SILVA_EXPECT_REQUIRE(frog_pt->span().to_string())};
    CHECK(frog_pt_str == expected.substr(1));
  }

  TEST_CASE("multiple-texts", "[seed_engine_t]")
  {
    const string_view_t text1_seed = R"'(
    - Foo = [
      - x = 'a' 'b' 'c' _.Bar ?
    ]
  )'";
    const string_view_t text2_seed = R"'(
    - Bar = [
      - Blub = 'u' 'v' 'w'
      - x = 'x' 'y' 'z' _.Foo ?
    ]
  )'";
    syntax_ward_t sw;
    seed_engine_t se(sw.ptr());
    SILVA_EXPECT_REQUIRE(se.add_complete_file("text1.seed", text1_seed));
    SILVA_EXPECT_REQUIRE(se.add_complete_file("text2.seed", text2_seed));

    const string_view_t code     = R"'(
    a b c x y z a b c
  )'";
    auto tt                      = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "", code));
    auto pt                      = SILVA_EXPECT_REQUIRE(se.apply(sw, tt, sw.name_id_of("Foo")));
    const string_view_t expected = R"(
[0]_.Foo                                          a b ... b c
  [0]_.Bar                                        x y ... b c
    [0]_.Foo                                      a b c
)";
    const string_t result_str{SILVA_EXPECT_REQUIRE(pt->span().to_string())};
    CHECK(result_str == expected.substr(1));
  }
}
