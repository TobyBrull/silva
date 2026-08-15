#include "seed.hpp"

#include "seed.globals.hpp"
#include "syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::seed::test {
  TEST_CASE("seed-parse-root", "[seed][seed::interpreter_t]")
  {
    syntax_farm_t sf;
    const auto spr = standard_seed_interpreter(sf.ptr());
    for (const auto txt: {seed_str, globals_str}) {
      const auto fp    = SILVA_REQUIRE(fragmentize(sf.ptr(), "sf.code", string_t{txt}));
      const auto pts_1 = SILVA_REQUIRE(bootstrap_interpreter_t{sf.ptr()}.parse(fp));
      const auto pts_2 = SILVA_REQUIRE(spr->apply(fp, sf.name_id_of("Seed")));
      CHECK(pts_1->nodes == pts_2->nodes);
    }
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
[0].Seed                                          strin ... ber<NEWLINE><DEDENT>
  [0].Seed.Rule                                   strin ... RING<NEWLINE>
    [0].Seed.Nonterminal                          string 
      [0].Seed.tokenCategoryName                  string
    [1].Seed.Terminal                             STRING
      [0].Seed.fragName                           STRING
  [1].Seed.Rule                                   numbe ... T +<NEWLINE><WHITESPACE>
    [0].Seed.Nonterminal                          number 
      [0].Seed.tokenCategoryName                  number
    [1].Seed.Expr.Postfix.+                       DIGIT +
      [0].Seed.Terminal                           DIGIT 
        [0].Seed.fragName                         DIGIT
      [1].Seed.Expr.operator                      +
  [2].Seed.Language                               langu ... ber<NEWLINE><DEDENT>
    [0].Seed.ruleName                             SimpleFern
    [1].Seed.Rule                                 ⊙ = '[' ... * ']'<NEWLINE><WHITESPACE>
      [0].Seed.here                               ⊙
      [1].Seed.Expr.Concat.concat                 '[' ( L ... ) * ']'
        [0].Seed.Terminal                         '[' 
          [0].string                              '['
        [1].Seed.Expr.Postfix.*                   ( Lab ...  ) * 
          [0].Seed.Expr.Concat.concat             Label ...  ';' ? 
            [0].Seed.Nonterminal                  Label ... Item 
              [0].Seed.ruleName                   Label ... dItem
            [1].Seed.Expr.Postfix.?               ';' ? 
              [0].Seed.Terminal                   ';' 
                [0].string                        ';'
              [1].Seed.Expr.operator              ?
          [1].Seed.Expr.operator                  *
        [2].Seed.Terminal                         ']'
          [0].string                              ']'
    [2].Seed.Rule                                 skip  ... ) *<NEWLINE><WHITESPACE>
      [0].Seed.Nonterminal                        skip 
        [0].Seed.tokenCategoryName                skip
      [1].Seed.Expr.Postfix.*                     ( SPA ... E ) *
        [0].Seed.Expr.Or.|                        SPACE ... LINE 
          [0].Seed.Terminal                       SPACE 
            [0].Seed.fragName                     SPACE
          [1].Seed.Expr.operator                  |
          [2].Seed.Terminal                       LINEFEED 
            [0].Seed.fragName                     LINEFEED
          [3].Seed.Expr.operator                  |
          [4].Seed.Terminal                       COMMENT 
            [0].Seed.fragName                     COMMENT
          [5].Seed.Expr.operator                  |
          [6].Seed.Terminal                       WHITE ... PACE 
            [0].Seed.fragName                     WHITESPACE
          [7].Seed.Expr.operator                  |
          [8].Seed.Terminal                       INDENT 
            [0].Seed.fragName                     INDENT
          [9].Seed.Expr.operator                  |
          [10].Seed.Terminal                      DEDENT 
            [0].Seed.fragName                     DEDENT
          [11].Seed.Expr.operator                 |
          [12].Seed.Terminal                      NEWLINE 
            [0].Seed.fragName                     NEWLINE
        [1].Seed.Expr.operator                    *
    [3].Seed.Rule                                 Label ... Item<NEWLINE>
      [0].Seed.Nonterminal                        Label ... Item 
        [0].Seed.ruleName                         Label ... dItem
      [1].Seed.Expr.Concat.concat                 ( Lab ...  Item
        [0].Seed.Expr.Postfix.?                   ( Lab ...  ) ? 
          [0].Seed.Expr.Concat.concat             Label ':' 
            [0].Seed.Nonterminal                  Label 
              [0].Seed.ruleName                   Label
            [1].Seed.Terminal                     ':' 
              [0].string                          ':'
          [1].Seed.Expr.operator                  ?
        [1].Seed.Nonterminal                      Item
          [0].Seed.ruleName                       Item
    [4].Seed.Rule                                 Label ... ring<NEWLINE>
      [0].Seed.Nonterminal                        Label 
        [0].Seed.ruleName                         Label
      [1].Seed.Nonterminal                        string
        [0].Seed.tokenCategoryName                string
    [5].Seed.Rule                                 Item  ... mber<NEWLINE>
      [0].Seed.Nonterminal                        Item 
        [0].Seed.ruleName                         Item
      [1].Seed.Expr.Or.|                          Simpl ... umber
        [0].Seed.Nonterminal                      Simpl ... Fern 
          [0].Seed.ruleName                       SimpleFern
        [1].Seed.Expr.operator                    |
        [2].Seed.Nonterminal                      string 
          [0].Seed.tokenCategoryName              string
        [3].Seed.Expr.operator                    |
        [4].Seed.Nonterminal                      number
          [0].Seed.tokenCategoryName              number
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
[0].SimpleFern                                    [ 'abc' ; ...  ;]<NEWLINE><DEDENT>
  [0].SimpleFern.LabeledItem                      'abc' 
    [0].SimpleFern.Item                           'abc' 
      [0].string                                  'abc'
  [1].SimpleFern.LabeledItem                      [ 'def' 123 ] 
    [0].SimpleFern.Item                           [ 'def' 123 ] 
      [0].SimpleFern                              [ 'def' 123 ] 
        [0].SimpleFern.LabeledItem                'def' 
          [0].SimpleFern.Item                     'def' 
            [0].string                            'def'
        [1].SimpleFern.LabeledItem                123 
          [0].SimpleFern.Item                     123 
            [0].number                            123
  [2].SimpleFern.LabeledItem                      'jkl' 
    [0].SimpleFern.Item                           'jkl' 
      [0].string                                  'jkl'
)";
      const string_t result{SILVA_REQUIRE(sfpt->span().to_string())};
      CHECK(result == expected_parse_tree.substr(1));
    }
  }

  TEST_CASE("tokenizers", "[seed][tokenizer]")
  {
    syntax_farm_t sf;
    const name_id_t id  = sf.name_id_of("identifier");
    const name_id_t num = sf.name_id_of("number");
    const name_id_t str = sf.name_id_of("string");
    const name_id_t boo = sf.name_id_of("boolean");
    const auto si       = standard_seed_interpreter(sf.ptr());

    const auto test = [&](string_t text,
                          const string_view_t rule,
                          array_t<string_view_t> expected_token_strs,
                          array_t<name_id_t> expected_rule_names) {
      INFO(text);
      const auto pts =
          SILVA_REQUIRE(si->apply_text("", std::move(text), sf.name_id_of(rule)))->span();
      const auto tok_ptses = pts.get_children_array();
      REQUIRE(tok_ptses.size() == expected_token_strs.size());
      REQUIRE(tok_ptses.size() == expected_rule_names.size());
      for (index_t i = 0; i < expected_rule_names.size(); ++i) {
        INFO(i);
        const token_id_t ti = SILVA_REQUIRE(tok_ptses[i].token());
        const name_id_t ni  = SILVA_REQUIRE(tok_ptses[i].iterate_to_child(0)).rule_name();
        CHECK(ti == sf.token_id(expected_token_strs[i]));
        CHECK(ni == expected_rule_names[i]);
      }
    };

    SECTION("test1")
    {
      SILVA_REQUIRE(si->add_seed_text("t.seed", R"'(
language Test:
  ⊙ = val *
  val = ( boolean | number | identifier )
  skip = skip.freeForm
)'"));

      test("ab 123ab\n", "Test", {"ab", "123", "ab"}, {id, num, id});
      test("truedat\n", "Test", {"truedat"}, {id});
      test("1 2 3\n", "Test", {"1", "2", "3"}, {num, num, num});
      test("0xff false foo\n", "Test", {"0xff", "false", "foo"}, {num, boo, id});
    }
  }
}
