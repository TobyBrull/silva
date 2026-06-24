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
  RuleName = alias Keyword
  Expr = Primary +
  Primary = not Keyword but_then identifier
  Keyword:
    ⊙ = 'keyword1' | 'keyword2' | 'keyword3'
)'";
    syntax_farm_t sf;
    interpreter_t se(sf.ptr());
    auto ptp = SILVA_REQUIRE(se.add_seed_text("frog.seed", string_t{frog_seed}));
    const string_view_t expected_seed_pt = R"(
[  0]   2:1   cat=.literal                                 language
[  1]   2:10  cat=.Seed.rule_name                          Frog
[  2]   2:14  cat=.literal                                 :
[  3]   2:15  cat=.newline                                 \n
[  4]   3:1   cat=.indent                                    
[  5]   3:3   cat=.literal                                 ⊙
[  6]   3:5   cat=.literal                                 =
[  7]   3:7   cat=.Seed.rule_name                          Rule
[  8]   3:12  cat=.Seed.Expr.operator                      *
[  9]   3:13  cat=.newline                                 \n  
[ 10]   4:3   cat=.Seed.token_category_name                skip
[ 11]   4:8   cat=.literal                                 =
[ 12]   4:10  cat=.literal                                 (
[ 13]   4:12  cat=.Seed.frag_name                          SPACE
[ 14]   4:18  cat=.Seed.Expr.operator                      |
[ 15]   4:20  cat=.Seed.frag_name                          LINEFEED
[ 16]   4:29  cat=.Seed.Expr.operator                      |
[ 17]   4:31  cat=.Seed.frag_name                          COMMENT
[ 18]   4:39  cat=.Seed.Expr.operator                      |
[ 19]   4:41  cat=.Seed.frag_name                          WHITESPACE
[ 20]   4:52  cat=.Seed.Expr.operator                      |
[ 21]   4:54  cat=.Seed.frag_name                          INDENT
[ 22]   4:61  cat=.Seed.Expr.operator                      |
[ 23]   4:63  cat=.Seed.frag_name                          DEDENT
[ 24]   4:70  cat=.Seed.Expr.operator                      |
[ 25]   4:72  cat=.Seed.frag_name                          NEWLINE
[ 26]   4:80  cat=.literal                                 )
[ 27]   4:82  cat=.Seed.Expr.operator                      *
[ 28]   4:83  cat=.newline                                 \n  
[ 29]   5:3   cat=.Seed.token_category_name                identifier
[ 30]   5:14  cat=.literal                                 =
[ 31]   5:16  cat=.Seed.frag_name                          ID_START
[ 32]   5:25  cat=.Seed.frag_name                          ID_CONTINUE
[ 33]   5:37  cat=.Seed.Expr.operator                      *
[ 34]   5:38  cat=.newline                                 \n  
[ 35]   6:3   cat=.Seed.rule_name                          Rule
[ 36]   6:8   cat=.literal                                 =
[ 37]   6:10  cat=.Seed.rule_name                          RuleName
[ 38]   6:19  cat=.Seed.rule_name                          Expr
[ 39]   6:23  cat=.newline                                 \n  
[ 40]   7:3   cat=.Seed.rule_name                          RuleName
[ 41]   7:12  cat=.literal                                 =
[ 42]   7:14  cat=.literal                                 alias
[ 43]   7:20  cat=.Seed.rule_name                          Keyword
[ 44]   7:27  cat=.newline                                 \n  
[ 45]   8:3   cat=.Seed.rule_name                          Expr
[ 46]   8:8   cat=.literal                                 =
[ 47]   8:10  cat=.Seed.rule_name                          Primary
[ 48]   8:18  cat=.Seed.Expr.operator                      +
[ 49]   8:19  cat=.newline                                 \n  
[ 50]   9:3   cat=.Seed.rule_name                          Primary
[ 51]   9:11  cat=.literal                                 =
[ 52]   9:13  cat=.Seed.Expr.operator                      not
[ 53]   9:17  cat=.Seed.rule_name                          Keyword
[ 54]   9:25  cat=.Seed.Expr.operator                      but_then
[ 55]   9:34  cat=.Seed.token_category_name                identifier
[ 56]   9:44  cat=.newline                                 \n  
[ 57]  10:3   cat=.Seed.rule_name                          Keyword
[ 58]  10:10  cat=.literal                                 :
[ 59]  10:11  cat=.newline                                 \n
[ 60]  11:1   cat=.indent                                      
[ 61]  11:5   cat=.literal                                 ⊙
[ 62]  11:7   cat=.literal                                 =
[ 63]  11:9   cat=.string                                  'keyword1'
[ 64]  11:20  cat=.Seed.Expr.operator                      |
[ 65]  11:22  cat=.string                                  'keyword2'
[ 66]  11:33  cat=.Seed.Expr.operator                      |
[ 67]  11:35  cat=.string                                  'keyword3'
[ 68]  11:45  cat=.newline                                 
[ 69]  11:45  cat=.dedent                                  
[ 70]  11:45  cat=.dedent                                  

[0].Seed                                          language Frog ...  
  [0].Seed.Language                               language Frog ...  
    [0].Seed.Rule                                 ⊙ = Rule * \n  
      [0].Seed.Expr.Postfix.*                     Rule *
        [0].Seed.Nonterminal                      Rule
    [1].Seed.Rule                                 skip = ... * \n  
      [0].Seed.Nonterminal                        skip
      [1].Seed.Expr.Postfix.*                     ( SPACE ... ) *
        [0].Seed.Expr.Or.|                        SPACE | ... | NEWLINE
          [0].Seed.Terminal                       SPACE
          [1].Seed.Terminal                       LINEFEED
          [2].Seed.Terminal                       COMMENT
          [3].Seed.Terminal                       WHITESPACE
          [4].Seed.Terminal                       INDENT
          [5].Seed.Terminal                       DEDENT
          [6].Seed.Terminal                       NEWLINE
    [2].Seed.Rule                                 identifier = ... * \n  
      [0].Seed.Nonterminal                        identifier
      [1].Seed.Expr.Concat.concat                 ID_START ID_CONTINUE *
        [0].Seed.Terminal                         ID_START
        [1].Seed.Expr.Postfix.*                   ID_CONTINUE *
          [0].Seed.Terminal                       ID_CONTINUE
    [3].Seed.Rule                                 Rule = RuleName Expr \n  
      [0].Seed.Nonterminal                        Rule
      [1].Seed.Expr.Concat.concat                 RuleName Expr
        [0].Seed.Nonterminal                      RuleName
        [1].Seed.Nonterminal                      Expr
    [4].Seed.Rule                                 RuleName = alias Keyword \n  
      [0].Seed.Nonterminal                        RuleName
      [1].Seed.Qualifier                          alias
      [2].Seed.Nonterminal                        Keyword
    [5].Seed.Rule                                 Expr = Primary + \n  
      [0].Seed.Nonterminal                        Expr
      [1].Seed.Expr.Postfix.+                     Primary +
        [0].Seed.Nonterminal                      Primary
    [6].Seed.Rule                                 Primary = ... identifier \n  
      [0].Seed.Nonterminal                        Primary
      [1].Seed.Expr.And.but_then                  not Keyword but_then identifier
        [0].Seed.Expr.Prefix.not                  not Keyword
          [0].Seed.Nonterminal                    Keyword
        [1].Seed.Nonterminal                      identifier
    [7].Seed.Scope                                Keyword : ...  
      [0].Seed.Nonterminal                        Keyword
      [1].Seed.Rule                               ⊙ = ... 'keyword3' 
        [0].Seed.Expr.Or.|                        'keyword1' | 'keyword2' | 'keyword3'
          [0].Seed.Terminal                       'keyword1'
          [1].Seed.Terminal                       'keyword2'
          [2].Seed.Terminal                       'keyword3'
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
[  0]   2:5   cat=.literal                                 keyword1
[  1]   2:14  cat=.Frog.identifier                         a
[  2]   2:16  cat=.Frog.identifier                         b
[  3]   2:18  cat=.Frog.identifier                         c
[  4]   3:5   cat=.literal                                 keyword2
[  5]   3:14  cat=.Frog.identifier                         d
[  6]   3:16  cat=.Frog.identifier                         e
[  7]   4:5   cat=.literal                                 keyword1
[  8]   4:14  cat=.Frog.identifier                         f
[  9]   5:5   cat=.literal                                 keyword3
[ 10]   5:14  cat=.Frog.identifier                         g
[ 11]   5:16  cat=.Frog.identifier                         h
[ 12]   5:18  cat=.Frog.identifier                         i

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
language Testor:
  ⊙ = Assign *
  skip = skip.free_form
  Assign = identifier '=' identifier operator_single identifier
)'";
    SILVA_REQUIRE(se->add_seed_text("testor.seed", string_t{testor_lang}));

    const string_view_t src = "x = a + b\ny = c * d\n";

    const auto fp = SILVA_REQUIRE(fragmentize(sf.ptr(), "test.src", string_t{src}));
    CHECK(fp->fragments.size() == 22);
    const auto pt = SILVA_REQUIRE(se->apply(fp, sf.name_id_of("Testor")));
    CHECK(pt->tp->size() == 10);

    const string_view_t expected = R"(
[  0]   1:1   cat=.identifier                              x
[  1]   1:3   cat=.literal                                 =
[  2]   1:5   cat=.identifier                              a
[  3]   1:7   cat=.operator_single                         +
[  4]   1:9   cat=.identifier                              b
[  5]   2:1   cat=.identifier                              y
[  6]   2:3   cat=.literal                                 =
[  7]   2:5   cat=.identifier                              c
[  8]   2:7   cat=.operator_single                         *
[  9]   2:9   cat=.identifier                              d

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
[  0]   2:5   cat=.literal                                 a
[  1]   2:7   cat=.literal                                 b
[  2]   2:9   cat=.literal                                 c
[  3]   2:11  cat=.literal                                 x
[  4]   2:13  cat=.literal                                 y
[  5]   2:15  cat=.literal                                 z
[  6]   2:17  cat=.literal                                 a
[  7]   2:19  cat=.literal                                 b
[  8]   2:21  cat=.literal                                 c

[0].Foo                                           a b ... b c
  [0].Bar                                         x y ... b c
    [0].Foo                                       a b c
)";
    const string_t result_str{SILVA_REQUIRE(pt->span().to_string())};
    CHECK(result_str == expected.substr(1));
  }
}
