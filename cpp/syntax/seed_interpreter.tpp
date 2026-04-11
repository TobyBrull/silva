#include "seed_interpreter.hpp"

#include "fragmentization.hpp"
#include "syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::seed::test {
  TEST_CASE("not-but_then", "[seed-interpreter][seed]")
  {
    const string_view_t frog_seed = R"'(
    - Frog = tokenizer [
      - ignore WHITESPACE
      - ignore COMMENT
      - ignore INDENT
      - ignore DEDENT
      - ignore NEWLINE
      - number = NUMBER
      - string = STRING
      - identifier = IDENTIFIER
    ]
    - Frog = [
      - ⊙ = Rule *
      - Rule = RuleName Expr
      - RuleName = alias Keyword
      - Expr = Primary +
      - Primary = not Keyword but_then identifier
      - Keyword = [
        - ⊙ = 'keyword1' | 'keyword2' | 'keyword3'
      ]
    ]
)'";
    syntax_farm_t sf;
    interpreter_t se(sf.ptr());
    auto ptp = SILVA_REQUIRE(se.add_seed_text("frog.seed", string_t{frog_seed}));
    const string_view_t expected_seed_pt = R"(
[0].Seed                                          - Frog ... ] ]
  [0].Seed.Rule                                   Frog = ... IDENTIFIER ]
    [0].Seed.Nonterminal                          Frog
      [0].Seed.Nonterminal.Base                   Frog
    [1].Seed.Tokenizer                            [ - ... IDENTIFIER ]
      [0].Seed.Tokenizer.IgnoreRule               ignore WHITESPACE
        [0].Seed.Tokenizer.Defn                   WHITESPACE
          [0].Seed.Tokenizer.PrefixItem           WHITESPACE
            [0].Seed.Tokenizer.Item               WHITESPACE
              [0].Seed.Tokenizer.Matcher          WHITESPACE
      [1].Seed.Tokenizer.IgnoreRule               ignore COMMENT
        [0].Seed.Tokenizer.Defn                   COMMENT
          [0].Seed.Tokenizer.PrefixItem           COMMENT
            [0].Seed.Tokenizer.Item               COMMENT
              [0].Seed.Tokenizer.Matcher          COMMENT
      [2].Seed.Tokenizer.IgnoreRule               ignore INDENT
        [0].Seed.Tokenizer.Defn                   INDENT
          [0].Seed.Tokenizer.PrefixItem           INDENT
            [0].Seed.Tokenizer.Item               INDENT
              [0].Seed.Tokenizer.Matcher          INDENT
      [3].Seed.Tokenizer.IgnoreRule               ignore DEDENT
        [0].Seed.Tokenizer.Defn                   DEDENT
          [0].Seed.Tokenizer.PrefixItem           DEDENT
            [0].Seed.Tokenizer.Item               DEDENT
              [0].Seed.Tokenizer.Matcher          DEDENT
      [4].Seed.Tokenizer.IgnoreRule               ignore NEWLINE
        [0].Seed.Tokenizer.Defn                   NEWLINE
          [0].Seed.Tokenizer.PrefixItem           NEWLINE
            [0].Seed.Tokenizer.Item               NEWLINE
              [0].Seed.Tokenizer.Matcher          NEWLINE
      [5].Seed.Tokenizer.TokenRule                number = NUMBER
        [0].Seed.Tokenizer.Defn                   NUMBER
          [0].Seed.Tokenizer.PrefixItem           NUMBER
            [0].Seed.Tokenizer.Item               NUMBER
              [0].Seed.Tokenizer.Matcher          NUMBER
      [6].Seed.Tokenizer.TokenRule                string = STRING
        [0].Seed.Tokenizer.Defn                   STRING
          [0].Seed.Tokenizer.PrefixItem           STRING
            [0].Seed.Tokenizer.Item               STRING
              [0].Seed.Tokenizer.Matcher          STRING
      [7].Seed.Tokenizer.TokenRule                identifier = IDENTIFIER
        [0].Seed.Tokenizer.Defn                   IDENTIFIER
          [0].Seed.Tokenizer.PrefixItem           IDENTIFIER
            [0].Seed.Tokenizer.Item               IDENTIFIER
              [0].Seed.Tokenizer.Matcher          IDENTIFIER
  [1].Seed.Rule                                   Frog = ... ] ]
    [0].Seed.Nonterminal                          Frog
      [0].Seed.Nonterminal.Base                   Frog
    [1].Seed                                      - ⊙ ... 'keyword3' ]
      [0].Seed.Rule                               ⊙ = Rule *
        [0].Seed.Nonterminal                      ⊙
          [0].Seed.Nonterminal.Base               ⊙
        [1].Seed.Expr.Postfix.*                   Rule *
          [0].Seed.Nonterminal                    Rule
            [0].Seed.Nonterminal.Base             Rule
      [1].Seed.Rule                               Rule = RuleName Expr
        [0].Seed.Nonterminal                      Rule
          [0].Seed.Nonterminal.Base               Rule
        [1].Seed.Expr.Concat.concat               RuleName Expr
          [0].Seed.Nonterminal                    RuleName
            [0].Seed.Nonterminal.Base             RuleName
          [1].Seed.Nonterminal                    Expr
            [0].Seed.Nonterminal.Base             Expr
      [2].Seed.Rule                               RuleName = alias Keyword
        [0].Seed.Nonterminal                      RuleName
          [0].Seed.Nonterminal.Base               RuleName
        [1].Seed.Alias                            Keyword
          [0].Seed.Nonterminal                    Keyword
            [0].Seed.Nonterminal.Base             Keyword
      [3].Seed.Rule                               Expr = Primary +
        [0].Seed.Nonterminal                      Expr
          [0].Seed.Nonterminal.Base               Expr
        [1].Seed.Expr.Postfix.+                   Primary +
          [0].Seed.Nonterminal                    Primary
            [0].Seed.Nonterminal.Base             Primary
      [4].Seed.Rule                               Primary = ... but_then identifier
        [0].Seed.Nonterminal                      Primary
          [0].Seed.Nonterminal.Base               Primary
        [1].Seed.Expr.And.but_then                not Keyword but_then identifier
          [0].Seed.Expr.Prefix.not                not Keyword
            [0].Seed.Nonterminal                  Keyword
              [0].Seed.Nonterminal.Base           Keyword
          [1].Seed.Terminal                       identifier
      [5].Seed.Rule                               Keyword = ... 'keyword3' ]
        [0].Seed.Nonterminal                      Keyword
          [0].Seed.Nonterminal.Base               Keyword
        [1].Seed                                  - ⊙ ... | 'keyword3'
          [0].Seed.Rule                           ⊙ = ... | 'keyword3'
            [0].Seed.Nonterminal                  ⊙
              [0].Seed.Nonterminal.Base           ⊙
            [1].Seed.Expr.Or.|                    'keyword1' | 'keyword2' | 'keyword3'
              [0].Seed.Terminal                   'keyword1'
              [1].Seed.Terminal                   'keyword2'
              [2].Seed.Terminal                   'keyword3'
)";
    const string_t seed_pt_str{SILVA_REQUIRE(ptp->span().to_string())};
    CHECK(seed_pt_str == expected_seed_pt.substr(1));

    const string_t frog_text = R"'(
    keyword1 a b c
    keyword2 d e
    keyword1 f
    keyword3 g h i
)'";
    const auto frog_pt       = SILVA_REQUIRE(se.apply_text("", frog_text, sf.name_id_of("Frog")));
    const string_view_t expected = R"(
[0].Frog                                          keyword1 a ... h i
  [0].Frog.Rule                                   keyword1 a b c
    [0].Frog.Keyword                              keyword1
    [1].Frog.Expr                                 a b c
      [0].Frog.Primary                            a
      [1].Frog.Primary                            b
      [2].Frog.Primary                            c
  [1].Frog.Rule                                   keyword2 d e
    [0].Frog.Keyword                              keyword2
    [1].Frog.Expr                                 d e
      [0].Frog.Primary                            d
      [1].Frog.Primary                            e
  [2].Frog.Rule                                   keyword1 f
    [0].Frog.Keyword                              keyword1
    [1].Frog.Expr                                 f
      [0].Frog.Primary                            f
  [3].Frog.Rule                                   keyword3 g h i
    [0].Frog.Keyword                              keyword3
    [1].Frog.Expr                                 g h i
      [0].Frog.Primary                            g
      [1].Frog.Primary                            h
      [2].Frog.Primary                            i
)";
    const string_t frog_pt_str{SILVA_REQUIRE(frog_pt->span().to_string())};
    CHECK(frog_pt_str == expected.substr(1));
  }

  TEST_CASE("apply-fragmentization", "[seed-interpreter]")
  {
    syntax_farm_t sf;
    auto se = standard_seed_interpreter(sf.ptr());

    const string_view_t testor_lang = R"'(
      - Testor = tokenizer [
        - ignore WHITESPACE
        - ignore COMMENT
        - ignore INDENT
        - ignore DEDENT
        - ignore NEWLINE
        - name = IDENTIFIER
        - op = OPERATOR
      ]
      - Testor = [
        - ⊙ = Assign *
        - Assign = name '=' name op name
      ]
)'";
    SILVA_REQUIRE(se->add_seed_text("testor.seed", string_t{testor_lang}));

    const string_view_t src = "x = a + b\ny = c * d\n";

    const auto fp = SILVA_REQUIRE(fragmentize(sf.ptr(), "test.src", string_t{src}));
    CHECK(fp->fragments.size() == 22);
    const auto pt = SILVA_REQUIRE(se->apply(fp, sf.name_id_of("Testor")));
    CHECK(pt->tp->size() == 10);

    const string_view_t expected = R"(
[0].Testor                                        x = ... * d
  [0].Testor.Assign                               x = a + b
  [1].Testor.Assign                               y = c * d
)";
    const string_t result        = SILVA_REQUIRE(pt->span().to_string());
    CHECK(result == expected.substr(1));
  }

  TEST_CASE("multiple-texts", "[seed-interpreter]")
  {
    const string_view_t text1_seed = R"'(
    - Foo = tokenizer [
      - ignore WHITESPACE
      - ignore COMMENT
      - ignore INDENT
      - ignore DEDENT
      - ignore NEWLINE
      - number = NUMBER
      - string = STRING
      - identifier = IDENTIFIER
    ]
    - Foo = [
      - ⊙ = 'a' 'b' 'c' Bar ?
    ]
)'";
    const string_view_t text2_seed = R"'(
    - Bar = [
      - Blub = 'u' 'v' 'w'
      - ⊙ = 'x' 'y' 'z' Foo ?
    ]
)'";
    syntax_farm_t sf;
    interpreter_t se(sf.ptr());
    SILVA_REQUIRE(se.add_seed_text("text1.seed", string_t{text1_seed}));
    SILVA_REQUIRE(se.add_seed_text("text2.seed", string_t{text2_seed}));

    const string_t text = R"'(
    a b c x y z a b c
)'";

    auto pt = SILVA_REQUIRE(se.apply_text("", text, sf.name_id_of("Foo")));

    const string_view_t expected = R"(
[0].Foo                                           a b ... b c
  [0].Bar                                         x y ... b c
    [0].Foo                                       a b c
)";
    const string_t result_str{SILVA_REQUIRE(pt->span().to_string())};
    CHECK(result_str == expected.substr(1));
  }
}
