#include "seed_interpreter.hpp"

#include "fragmentization.hpp"
#include "syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::seed::test {
  TEST_CASE("not-but_then", "[seed-interpreter][seed]")
  {
    const string_view_t frog_seed = R"'(
language Frog:
  ⊙ = Rule *
  skip = ( SPACE | LINEFEED | COMMENT | WHITESPACE | INDENT | DEDENT | NEWLINE ) *
  identifier = ID_START ID_CONTINUE *
  Rule = RuleName Expr
  RuleName = no_node Keyword
  Expr = Primary +
  Primary = not literals_of Keyword but_then identifier
  Keyword:
    ⊙ = 'keyword1' | 'keyword2' | 'keyword3'
)'";
    syntax_farm_t sf;
    interpreter_t se(sf.ptr());
    auto ptp = SILVA_REQUIRE(se.add_seed_text("frog.seed", string_t{frog_seed}));
    const string_view_t expected_seed_pt = R"(
[0].Seed                                          langu ...  'keyword3'<NEWLINE><DEDENT><DEDENT>¦
  [0].Seed.Language                               langu ...  'keyword3'<NEWLINE><DEDENT><DEDENT>¦
    [0].Seed.ruleName                             ｢Frog｣
    [1].Seed.Rule                                 ⊙ = R ... le *<NEWLINE>¦
      [0].Seed.here                               ｢⊙｣
      [1].Seed.Expr                               Rule *¦
        [0].Seed.Expr.Postfix.*                   Rule *¦
          [0].Seed.Nonterminal                    Rule ¦
            [0].Seed.ruleName                     ｢Rule｣
    [2].Seed.Rule                                 skip  ...  ) *<NEWLINE>¦
      [0].Seed.Nonterminal                        skip ¦
        [0].Seed.tokenCategoryName                ｢skip｣
      [1].Seed.Expr                               ( SPA ... E ) *¦
        [0].Seed.Expr.Postfix.*                   ( SPA ... E ) *¦
          [0].Seed.Expr                           SPACE ... LINE ¦
            [0].Seed.Expr.Or.|                    SPACE ... LINE ¦
              [0].Seed.Terminal                   SPACE ¦
                [0].Seed.fragName                 ｢SPACE｣
              [1].Seed.Terminal                   LINEFEED ¦
                [0].Seed.fragName                 ｢LINEFEED｣
              [2].Seed.Terminal                   COMMENT ¦
                [0].Seed.fragName                 ｢COMMENT｣
              [3].Seed.Terminal                   WHITE ... PACE ¦
                [0].Seed.fragName                 ｢WHITESPACE｣
              [4].Seed.Terminal                   INDENT ¦
                [0].Seed.fragName                 ｢INDENT｣
              [5].Seed.Terminal                   DEDENT ¦
                [0].Seed.fragName                 ｢DEDENT｣
              [6].Seed.Terminal                   NEWLINE ¦
                [0].Seed.fragName                 ｢NEWLINE｣
    [3].Seed.Rule                                 ident ... UE *<NEWLINE>¦
      [0].Seed.Nonterminal                        ident ... fier ¦
        [0].Seed.tokenCategoryName                ｢identifier｣
      [1].Seed.Expr                               ID_ST ... NUE *¦
        [0].Seed.Expr.Concat.concat               ID_ST ... NUE *¦
          [0].Seed.Terminal                       ID_START ¦
            [0].Seed.fragName                     ｢ID_START｣
          [1].Seed.Expr.Postfix.*                 ID_CO ... NUE *¦
            [0].Seed.Terminal                     ID_CO ... INUE ¦
              [0].Seed.fragName                   ｢ID_CONTINUE｣
    [4].Seed.Rule                                 Rule  ... Expr<NEWLINE>¦
      [0].Seed.Nonterminal                        Rule ¦
        [0].Seed.ruleName                         ｢Rule｣
      [1].Seed.Expr                               RuleN ...  Expr¦
        [0].Seed.Expr.Concat.concat               RuleN ...  Expr¦
          [0].Seed.Nonterminal                    RuleName ¦
            [0].Seed.ruleName                     ｢RuleName｣
          [1].Seed.Nonterminal                    Expr¦
            [0].Seed.ruleName                     ｢Expr｣
    [5].Seed.Rule                                 RuleN ... word<NEWLINE>¦
      [0].Seed.Nonterminal                        RuleName ¦
        [0].Seed.ruleName                         ｢RuleName｣
      [1].Seed.qualifier                          ｢no_node｣
      [2].Seed.Expr                               Keyword¦
        [0].Seed.Nonterminal                      Keyword¦
          [0].Seed.ruleName                       ｢Keyword｣
    [6].Seed.Rule                                 Expr  ... ry +<NEWLINE>¦
      [0].Seed.Nonterminal                        Expr ¦
        [0].Seed.ruleName                         ｢Expr｣
      [1].Seed.Expr                               Primary +¦
        [0].Seed.Expr.Postfix.+                   Primary +¦
          [0].Seed.Nonterminal                    Primary ¦
            [0].Seed.ruleName                     ｢Primary｣
    [7].Seed.Rule                                 Prima ... fier<NEWLINE>¦
      [0].Seed.Nonterminal                        Primary ¦
        [0].Seed.ruleName                         ｢Primary｣
      [1].Seed.Expr                               not l ... ifier¦
        [0].Seed.Expr.And.but_then                not l ... ifier¦
          [0].Seed.Expr.Prefix.not                not l ... word ¦
            [0].Seed.Terminal                     liter ... word ¦
              [0].Seed.Nonterminal                Keyword ¦
                [0].Seed.ruleName                 ｢Keyword｣
          [1].Seed.Nonterminal                    identifier¦
            [0].Seed.tokenCategoryName            ｢identifier｣
    [8].Seed.Scope                                Keywo ... | 'keyword3'<NEWLINE><DEDENT>¦
      [0].Seed.Nonterminal                        Keyword¦
        [0].Seed.ruleName                         ｢Keyword｣
      [1].Seed.Rule                               ⊙ = 'keyword1' ...  | 'keyword3'<NEWLINE>¦
        [0].Seed.here                             ｢⊙｣
        [1].Seed.Expr                             'keyword1' | 'keyword2' | 'keyword3'¦
          [0].Seed.Expr.Or.|                      'keyword1' | 'keyword2' | 'keyword3'¦
            [0].Seed.Terminal                     'keyword1' ¦
              [0].string                          ｢'keyword1'｣
            [1].Seed.Terminal                     'keyword2' ¦
              [0].string                          ｢'keyword2'｣
            [2].Seed.Terminal                     'keyword3'¦
              [0].string                          ｢'keyword3'｣
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
[0].Frog                                          keywo ... h i<NEWLINE><DEDENT>¦
  [0].Frog.Rule                                   keywo ...  b c<NEWLINE>¦
    [0].Frog.Keyword                              keyword1 ¦
    [1].Frog.Expr                                 a b c<NEWLINE>¦
      [0].Frog.Primary                            a ¦
        [0].Frog.identifier                       ｢a｣
      [1].Frog.Primary                            b ¦
        [0].Frog.identifier                       ｢b｣
      [2].Frog.Primary                            c<NEWLINE>¦
        [0].Frog.identifier                       ｢c｣
  [1].Frog.Rule                                   keywo ...  d e<NEWLINE>¦
    [0].Frog.Keyword                              keyword2 ¦
    [1].Frog.Expr                                 d e<NEWLINE>¦
      [0].Frog.Primary                            d ¦
        [0].Frog.identifier                       ｢d｣
      [1].Frog.Primary                            e<NEWLINE>¦
        [0].Frog.identifier                       ｢e｣
  [2].Frog.Rule                                   keywo ... d1 f<NEWLINE>¦
    [0].Frog.Keyword                              keyword1 ¦
    [1].Frog.Expr                                 f<NEWLINE>¦
      [0].Frog.Primary                            f<NEWLINE>¦
        [0].Frog.identifier                       ｢f｣
  [3].Frog.Rule                                   keywo ... h i<NEWLINE><DEDENT>¦
    [0].Frog.Keyword                              keyword3 ¦
    [1].Frog.Expr                                 g h i<NEWLINE><DEDENT>¦
      [0].Frog.Primary                            g ¦
        [0].Frog.identifier                       ｢g｣
      [1].Frog.Primary                            h ¦
        [0].Frog.identifier                       ｢h｣
      [2].Frog.Primary                            i<NEWLINE><DEDENT>¦
        [0].Frog.identifier                       ｢i｣
)";
    const string_t frog_pt_str{SILVA_REQUIRE(frog_pt->span().to_string())};
    CHECK(frog_pt_str == expected.substr(1));
  }

  TEST_CASE("apply-fragmentization", "[seed-interpreter]")
  {
    syntax_farm_t sf;
    auto se = standard_seed_interpreter(sf.ptr());

    const string_view_t testor_lang = R"'(
language Testor:
  ⊙ = Assign *
  skip = skip.freeForm
  Assign = identifier '=' identifier operator.single identifier
)'";
    SILVA_REQUIRE(se->add_seed_text("testor.seed", string_t{testor_lang}));

    const string_view_t src = "x = a + b\ny = c * d\n";

    const auto fp = SILVA_REQUIRE(fragmentize(sf.ptr(), "test.src", string_t{src}));
    CHECK(fp->fragments.size() == 22);
    const auto pt = SILVA_REQUIRE(se->apply(fp, sf.name_id_of("Testor")));

    const string_view_t expected = R"(
[0].Testor                                        x = a ...  * d<NEWLINE>¦
  [0].Testor.Assign                               x = a + b<NEWLINE>¦
    [0].identifier                                ｢x｣
    [1].identifier                                ｢a｣
    [2].identifier                                ｢b｣
  [1].Testor.Assign                               y = c * d<NEWLINE>¦
    [0].identifier                                ｢y｣
    [1].identifier                                ｢c｣
    [2].identifier                                ｢d｣
)";
    const string_t result        = SILVA_REQUIRE(pt->span().to_string());
    CHECK(result == expected.substr(1));
  }

  TEST_CASE("commit", "[seed-interpreter]")
  {
    const string_view_t testor_seed = R"'(
language Testor:
  skip = skip.freeForm
  ⊙ = Plain
  Plain          =   "static" "func" identifier | "static" identifier number
  NoPrefix       = ε "static" "func" identifier | "static" identifier number
  NoPrefixCommit = ε "static" "func" commit identifier | "static" identifier number
  EarlyCommit    =   "static" commit "func" identifier | "static" identifier number
)'";
    syntax_farm_t sf;
    auto se = standard_seed_interpreter(sf.ptr());
    SILVA_REQUIRE(se->add_seed_text("testor.seed", string_t{testor_seed}));

    const name_id_t ni_plain            = sf.name_id_of("Testor", "Plain");
    const name_id_t ni_no_prefix        = sf.name_id_of("Testor", "NoPrefix");
    const name_id_t ni_no_prefix_commit = sf.name_id_of("Testor", "NoPrefixCommit");
    const name_id_t ni_early_commit     = sf.name_id_of("Testor", "EarlyCommit");

    SILVA_REQUIRE(se->apply_text("", "static class 42\n", ni_plain));
    SILVA_REQUIRE(se->apply_text("", "static class 42\n", ni_no_prefix));
    SILVA_REQUIRE(se->apply_text("", "static class 42\n", ni_no_prefix_commit));
    SILVA_REQUIRE_ERROR(se->apply_text("", "static class 42\n", ni_early_commit));

    SILVA_REQUIRE_ERROR(se->apply_text("", "static func 42\n", ni_plain));
    SILVA_REQUIRE(se->apply_text("", "static func 42\n", ni_no_prefix));
    SILVA_REQUIRE_ERROR(se->apply_text("", "static func 42\n", ni_no_prefix_commit));
    SILVA_REQUIRE_ERROR(se->apply_text("", "static func 42\n", ni_early_commit));
  }

  TEST_CASE("multiple-texts", "[seed-interpreter]")
  {
    const string_view_t text1_seed = R"'(
language Foo:
  ⊙ = 'a' 'b' 'c' Bar ?
  skip = ( SPACE | LINEFEED | COMMENT | WHITESPACE | INDENT | DEDENT | NEWLINE ) *
)'";
    const string_view_t text2_seed = R"'(
Bar:
  ⊙ = 'x' 'y' 'z' Foo ?
  Blub = 'u' 'v' 'w'
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
[0].Foo                                           a b c ... b c<NEWLINE><DEDENT>¦
  [0].Bar                                         x y z ... b c<NEWLINE><DEDENT>¦
    [0].Foo                                       a b c<NEWLINE><DEDENT>¦
)";
    const string_t result_str{SILVA_REQUIRE(pt->span().to_string())};
    CHECK(result_str == expected.substr(1));
  }
}
