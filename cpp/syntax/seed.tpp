#include "seed.hpp"

#include "syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::seed::test {
  TEST_CASE("seed-parse-root", "[seed][seed::interpreter_t]")
  {
    syntax_farm_t sf;
    const auto spr   = standard_seed_interpreter(sf.ptr());
    const auto fp    = SILVA_REQUIRE(fragmentize(sf.ptr(), "sf.code", string_t{seed_str}));
    const auto pts_1 = SILVA_REQUIRE(bootstrap_interpreter_t{sf.ptr()}.parse(fp));
    const auto pts_2 = SILVA_REQUIRE(spr->apply(fp, sf.name_id_of("Seed")));
    CHECK(pts_1->nodes == pts_2->nodes);
  }

  TEST_CASE("seed", "[seed][seed::interpreter_t]")
  {
    const string_t sf_text = R"'(
language SimpleFern:
  ⊙ = '[' ( LabeledItem ';' ? ) * ']'

  skip = skip_off_side

  LabeledItem = ( Label ':' ) ? Item
  Label = string
  Item = SimpleFern | string | number
)'";
    syntax_farm_t sf;
    const auto spr   = standard_seed_interpreter(sf.ptr());
    const auto fp    = SILVA_REQUIRE(fragmentize(sf.ptr(), "sf.seed", string_t{sf_text}));
    const auto pts_1 = SILVA_REQUIRE(bootstrap_interpreter_t{sf.ptr()}.parse(fp));
    const auto pts_2 = SILVA_REQUIRE(spr->apply(fp, sf.name_id_of("Seed")));
    CHECK(pts_1->nodes == pts_2->nodes);
    const std::string_view expected = R"(
[  0]   2:1   cat=.literal                                 language
[  1]   2:10  cat=.Seed.rule_name                          SimpleFern
[  2]   2:20  cat=.literal                                 :
[  3]   2:21  cat=.newline                                 \n
[  4]   3:1   cat=.indent                                    
[  5]   3:3   cat=.literal                                 ⊙
[  6]   3:5   cat=.literal                                 =
[  7]   3:7   cat=.string                                  '['
[  8]   3:11  cat=.literal                                 (
[  9]   3:13  cat=.Seed.rule_name                          LabeledItem
[ 10]   3:25  cat=.string                                  ';'
[ 11]   3:29  cat=.Seed.Expr.operator                      ?
[ 12]   3:31  cat=.literal                                 )
[ 13]   3:33  cat=.Seed.Expr.operator                      *
[ 14]   3:35  cat=.string                                  ']'
[ 15]   3:38  cat=.newline                                 \n
[ 16]   5:3   cat=.Seed.token_category_name                skip
[ 17]   5:8   cat=.literal                                 =
[ 18]   5:10  cat=.Seed.token_category_name                skip_off_side
[ 19]   5:23  cat=.newline                                 \n
[ 20]   7:3   cat=.Seed.rule_name                          LabeledItem
[ 21]   7:15  cat=.literal                                 =
[ 22]   7:17  cat=.literal                                 (
[ 23]   7:19  cat=.Seed.rule_name                          Label
[ 24]   7:25  cat=.string                                  ':'
[ 25]   7:29  cat=.literal                                 )
[ 26]   7:31  cat=.Seed.Expr.operator                      ?
[ 27]   7:33  cat=.Seed.rule_name                          Item
[ 28]   7:37  cat=.newline                                 \n  
[ 29]   8:3   cat=.Seed.rule_name                          Label
[ 30]   8:9   cat=.literal                                 =
[ 31]   8:11  cat=.Seed.token_category_name                string
[ 32]   8:17  cat=.newline                                 \n  
[ 33]   9:3   cat=.Seed.rule_name                          Item
[ 34]   9:8   cat=.literal                                 =
[ 35]   9:10  cat=.Seed.rule_name                          SimpleFern
[ 36]   9:21  cat=.Seed.Expr.operator                      |
[ 37]   9:23  cat=.Seed.token_category_name                string
[ 38]   9:30  cat=.Seed.Expr.operator                      |
[ 39]   9:32  cat=.Seed.token_category_name                number
[ 40]   9:38  cat=.newline                                 
[ 41]   9:38  cat=.dedent                                  

[0].Seed                                          language SimpleFern ...  
  [0].Seed.Language                               language SimpleFern ...  
    [0].Seed.Rule                                 ⊙ = ... ']' \n
      [0].Seed.Expr.Concat.concat                 '[' ( ... * ']'
        [0].Seed.Terminal                         '['
        [1].Seed.Expr.Postfix.*                   ( LabeledItem ... ) *
          [0].Seed.Expr.Concat.concat             LabeledItem ';' ?
            [0].Seed.Nonterminal                  LabeledItem
            [1].Seed.Expr.Postfix.?               ';' ?
              [0].Seed.Terminal                   ';'
        [2].Seed.Terminal                         ']'
    [1].Seed.Rule                                 skip = skip_off_side \n
      [0].Seed.Nonterminal                        skip
      [1].Seed.Nonterminal                        skip_off_side
    [2].Seed.Rule                                 LabeledItem = ... Item \n  
      [0].Seed.Nonterminal                        LabeledItem
      [1].Seed.Expr.Concat.concat                 ( Label ... ? Item
        [0].Seed.Expr.Postfix.?                   ( Label ':' ) ?
          [0].Seed.Expr.Concat.concat             Label ':'
            [0].Seed.Nonterminal                  Label
            [1].Seed.Terminal                     ':'
        [1].Seed.Nonterminal                      Item
    [3].Seed.Rule                                 Label = string \n  
      [0].Seed.Nonterminal                        Label
      [1].Seed.Nonterminal                        string
    [4].Seed.Rule                                 Item = ... number 
      [0].Seed.Nonterminal                        Item
      [1].Seed.Expr.Or.|                          SimpleFern | string | number
        [0].Seed.Nonterminal                      SimpleFern
        [1].Seed.Nonterminal                      string
        [2].Seed.Nonterminal                      number
)";

    const string_t pts_1_str = SILVA_REQUIRE(pts_1->span().to_string());
    const string_t pts_2_str = SILVA_REQUIRE(pts_2->span().to_string());
    CHECK(pts_1_str == expected.substr(1));
    CHECK(pts_2_str == expected.substr(1));

    {
      interpreter_t se(sf.ptr());
      SILVA_REQUIRE(se.add_seed(pts_1->span()));
      const string_t sf_code = " [ 'abc' ; [ 'def' 123 ] 'jkl' ;]\n";
      const auto fp          = SILVA_REQUIRE(fragmentize(sf.ptr(), "sf.code", sf_code));
      const auto sfpt        = SILVA_REQUIRE(se.apply(fp, sf.name_id_of("SimpleFern")));
      const std::string_view expected_parse_tree = R"(
[0].SimpleFern                                    [ 'abc' ... ; ]
  [0].SimpleFern.LabeledItem                      'abc'
    [0].SimpleFern.Item                           'abc'
  [1].SimpleFern.LabeledItem                      [ 'def' 123 ]
    [0].SimpleFern.Item                           [ 'def' 123 ]
      [0].SimpleFern                              [ 'def' 123 ]
        [0].SimpleFern.LabeledItem                'def'
          [0].SimpleFern.Item                     'def'
        [1].SimpleFern.LabeledItem                123
          [0].SimpleFern.Item                     123
  [2].SimpleFern.LabeledItem                      'jkl'
    [0].SimpleFern.Item                           'jkl'
)";
      const string_t result{SILVA_REQUIRE(sfpt->span().to_string())};
      CHECK(result == expected_parse_tree.substr(1));
    }
  }
}
