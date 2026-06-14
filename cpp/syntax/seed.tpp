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
string = STRING
number = DIGIT +

language SimpleFern:
  ⊙ = '[' ( LabeledItem ';' ? ) * ']'

  skip = ( SPACE | LINEFEED | COMMENT | WHITESPACE | INDENT | DEDENT | NEWLINE ) *

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
[  0]   2:1   cat=.Seed.token_category_name                string
[  1]   2:8   cat=.literal                                 =
[  2]   2:10  cat=.Seed.frag_name                          STRING
[  3]   2:16  cat=.newline                                 \n
[  4]   3:1   cat=.Seed.token_category_name                number
[  5]   3:8   cat=.literal                                 =
[  6]   3:10  cat=.Seed.frag_name                          DIGIT
[  7]   3:16  cat=.Seed.Expr.operator                      +
[  8]   3:17  cat=.newline                                 \n
[  9]   5:1   cat=.literal                                 language
[ 10]   5:10  cat=.Seed.rule_name                          SimpleFern
[ 11]   5:20  cat=.literal                                 :
[ 12]   5:21  cat=.newline                                 \n
[ 13]   6:1   cat=.indent                                    
[ 14]   6:3   cat=.literal                                 ⊙
[ 15]   6:5   cat=.literal                                 =
[ 16]   6:7   cat=.string                                  '['
[ 17]   6:11  cat=.literal                                 (
[ 18]   6:13  cat=.Seed.rule_name                          LabeledItem
[ 19]   6:25  cat=.string                                  ';'
[ 20]   6:29  cat=.Seed.Expr.operator                      ?
[ 21]   6:31  cat=.literal                                 )
[ 22]   6:33  cat=.Seed.Expr.operator                      *
[ 23]   6:35  cat=.string                                  ']'
[ 24]   6:38  cat=.newline                                 \n
[ 25]   8:3   cat=.Seed.token_category_name                skip
[ 26]   8:8   cat=.literal                                 =
[ 27]   8:10  cat=.literal                                 (
[ 28]   8:12  cat=.Seed.frag_name                          SPACE
[ 29]   8:18  cat=.Seed.Expr.operator                      |
[ 30]   8:20  cat=.Seed.frag_name                          LINEFEED
[ 31]   8:29  cat=.Seed.Expr.operator                      |
[ 32]   8:31  cat=.Seed.frag_name                          COMMENT
[ 33]   8:39  cat=.Seed.Expr.operator                      |
[ 34]   8:41  cat=.Seed.frag_name                          WHITESPACE
[ 35]   8:52  cat=.Seed.Expr.operator                      |
[ 36]   8:54  cat=.Seed.frag_name                          INDENT
[ 37]   8:61  cat=.Seed.Expr.operator                      |
[ 38]   8:63  cat=.Seed.frag_name                          DEDENT
[ 39]   8:70  cat=.Seed.Expr.operator                      |
[ 40]   8:72  cat=.Seed.frag_name                          NEWLINE
[ 41]   8:80  cat=.literal                                 )
[ 42]   8:82  cat=.Seed.Expr.operator                      *
[ 43]   8:83  cat=.newline                                 \n
[ 44]  10:3   cat=.Seed.rule_name                          LabeledItem
[ 45]  10:15  cat=.literal                                 =
[ 46]  10:17  cat=.literal                                 (
[ 47]  10:19  cat=.Seed.rule_name                          Label
[ 48]  10:25  cat=.string                                  ':'
[ 49]  10:29  cat=.literal                                 )
[ 50]  10:31  cat=.Seed.Expr.operator                      ?
[ 51]  10:33  cat=.Seed.rule_name                          Item
[ 52]  10:37  cat=.newline                                 \n  
[ 53]  11:3   cat=.Seed.rule_name                          Label
[ 54]  11:9   cat=.literal                                 =
[ 55]  11:11  cat=.Seed.token_category_name                string
[ 56]  11:17  cat=.newline                                 \n  
[ 57]  12:3   cat=.Seed.rule_name                          Item
[ 58]  12:8   cat=.literal                                 =
[ 59]  12:10  cat=.Seed.rule_name                          SimpleFern
[ 60]  12:21  cat=.Seed.Expr.operator                      |
[ 61]  12:23  cat=.Seed.token_category_name                string
[ 62]  12:30  cat=.Seed.Expr.operator                      |
[ 63]  12:32  cat=.Seed.token_category_name                number
[ 64]  12:38  cat=.newline                                 
[ 65]  12:38  cat=.dedent                                  

[0].Seed                                          string = ...  
  [0].Seed.Rule                                   string = STRING \n
    [0].Seed.Nonterminal                          string
    [1].Seed.Terminal                             STRING
  [1].Seed.Rule                                   number = DIGIT + \n
    [0].Seed.Nonterminal                          number
    [1].Seed.Expr.Postfix.+                       DIGIT +
      [0].Seed.Terminal                           DIGIT
  [2].Seed.Language                               language SimpleFern ...  
    [0].Seed.Rule                                 ⊙ = ... ']' \n
      [0].Seed.Expr.Concat.concat                 '[' ( ... * ']'
        [0].Seed.Terminal                         '['
        [1].Seed.Expr.Postfix.*                   ( LabeledItem ... ) *
          [0].Seed.Expr.Concat.concat             LabeledItem ';' ?
            [0].Seed.Nonterminal                  LabeledItem
            [1].Seed.Expr.Postfix.?               ';' ?
              [0].Seed.Terminal                   ';'
        [2].Seed.Terminal                         ']'
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
[  0]   1:2   cat=.literal                                 [
[  1]   1:4   cat=.string                                  'abc'
[  2]   1:10  cat=.literal                                 ;
[  3]   1:12  cat=.literal                                 [
[  4]   1:14  cat=.string                                  'def'
[  5]   1:20  cat=.number                                  123
[  6]   1:24  cat=.literal                                 ]
[  7]   1:26  cat=.string                                  'jkl'
[  8]   1:32  cat=.literal                                 ;
[  9]   1:33  cat=.literal                                 ]

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
