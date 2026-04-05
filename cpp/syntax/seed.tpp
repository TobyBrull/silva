#include "seed.hpp"

#include "name_id_style.hpp"
#include "syntax.hpp"

#include <catch2/catch_all.hpp>

namespace silva::seed::test {
  TEST_CASE("name-id-style", "[name_id_style_t]")
  {
    syntax_farm_t sf;

    const name_id_t name1 = sf.name_id_of("std", "expr", "stmt");
    const name_id_t name2 = sf.name_id_of("std", "expr");
    const name_id_t name3 = sf.name_id_of("std", "ranges", "vector");

    const name_id_style_t ts{
        .sfp       = sf.ptr(),
        .current   = sf.token_id("this"),
        .separator = sf.token_id("::"),
    };
    CHECK(ts.absolute(name1) == "::std::expr::stmt");
    CHECK(ts.absolute(name2) == "::std::expr");
    CHECK(ts.absolute(name3) == "::std::ranges::vector");
  }

  TEST_CASE("seed-parse-root", "[seed][seed::interpreter_t]")
  {
    syntax_farm_t sf;
    const auto spr   = standard_seed_interpreter(sf.ptr());
    const auto fp    = SILVA_REQUIRE(fragmentize(sf.ptr(), "sf.code", string_t{seed_str}));
    const auto pts_1 = SILVA_REQUIRE(bootstrap_interpreter_t{sf.ptr()}.parse(fp));
    const auto pts_2 = SILVA_REQUIRE(spr->apply(fp, sf.name_id_of("Seed")));
    CHECK(pts_1->nodes == pts_2->nodes);
    CHECK(spr->keyword_scopes[sf.name_id_of("Seed", "Rule")] == hash_set_t<token_id_t>({}));
    CHECK(spr->keyword_scopes[sf.name_id_of("Seed", "Axe")] ==
          hash_set_t<token_id_t>({
              sf.token_id("["),
              sf.token_id("]"),
              sf.token_id("-"),
              sf.token_id("="),
              sf.token_id("nest"),
              sf.token_id("ltr"),
              sf.token_id("rtl"),
              sf.token_id("atom_nest"),
              sf.token_id("atom_nest_transparent"),
              sf.token_id("prefix"),
              sf.token_id("prefix_nest"),
              sf.token_id("infix"),
              sf.token_id("infix_flat"),
              sf.token_id("ternary"),
              sf.token_id("postfix"),
              sf.token_id("postfix_nest"),
              sf.token_id("concat"),
              sf.token_id("->"),
          }));
  }

  TEST_CASE("seed", "[seed][seed::interpreter_t]")
  {
    const string_t sf_text = R"'(
    - SimpleFern = tokenizer [
      - ignore WHITESPACE
      - ignore COMMENT
      - ignore INDENT
      - ignore DEDENT
      - ignore NEWLINE
      - number = NUMBER
      - string = STRING
      - operator = PARENTHESIS
      - operator = ::: OPERATOR
      - identifier = IDENTIFIER
    ]
    - SimpleFern = [
      - ⊙ = '[' ( LabeledItem ';' ? ) * ']'
      - LabeledItem = ( Label ':' )? Item
      - Label = string
      - Item = ⊙ | string | number
    ]
)'";
    syntax_farm_t sf;
    const auto spr   = standard_seed_interpreter(sf.ptr());
    const auto fp    = SILVA_REQUIRE(fragmentize(sf.ptr(), "sf.seed", string_t{sf_text}));
    const auto pts_1 = SILVA_REQUIRE(bootstrap_interpreter_t{sf.ptr()}.parse(fp));
    const auto pts_2 = SILVA_REQUIRE(spr->apply(fp, sf.name_id_of("Seed")));
    CHECK(pts_1->nodes == pts_2->nodes);
    const std::string_view expected = R"(
[0].Seed                                          - SimpleFern ... number ]
  [0].Seed.Rule                                   SimpleFern = ... IDENTIFIER ]
    [0].Seed.Nonterminal                          SimpleFern
      [0].Seed.Nonterminal.Base                   SimpleFern
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
      [7].Seed.Tokenizer.TokenRule                operator = PARENTHESIS
        [0].Seed.Tokenizer.Defn                   PARENTHESIS
          [0].Seed.Tokenizer.PrefixItem           PARENTHESIS
            [0].Seed.Tokenizer.Item               PARENTHESIS
              [0].Seed.Tokenizer.Matcher          PARENTHESIS
      [8].Seed.Tokenizer.TokenRule                operator = ::: OPERATOR
        [0].Seed.Tokenizer.Defn                   ::: OPERATOR
          [0].Seed.Tokenizer.Item                 OPERATOR
            [0].Seed.Tokenizer.Matcher            OPERATOR
      [9].Seed.Tokenizer.TokenRule                identifier = IDENTIFIER
        [0].Seed.Tokenizer.Defn                   IDENTIFIER
          [0].Seed.Tokenizer.PrefixItem           IDENTIFIER
            [0].Seed.Tokenizer.Item               IDENTIFIER
              [0].Seed.Tokenizer.Matcher          IDENTIFIER
  [1].Seed.Rule                                   SimpleFern = ... number ]
    [0].Seed.Nonterminal                          SimpleFern
      [0].Seed.Nonterminal.Base                   SimpleFern
    [1].Seed                                      - ⊙ ... | number
      [0].Seed.Rule                               ⊙ = ... * ']'
        [0].Seed.Nonterminal                      ⊙
          [0].Seed.Nonterminal.Base               ⊙
        [1].Seed.Expr.Concat.concat               '[' ( ... * ']'
          [0].Seed.Terminal                       '['
          [1].Seed.Expr.Postfix.*                 ( LabeledItem ... ) *
            [0].Seed.Expr.Parens.(                ( LabeledItem ';' ? )
              [0].Seed.Expr.Concat.concat         LabeledItem ';' ?
                [0].Seed.Nonterminal              LabeledItem
                  [0].Seed.Nonterminal.Base       LabeledItem
                [1].Seed.Expr.Postfix.?           ';' ?
                  [0].Seed.Terminal               ';'
          [2].Seed.Terminal                       ']'
      [1].Seed.Rule                               LabeledItem = ... ? Item
        [0].Seed.Nonterminal                      LabeledItem
          [0].Seed.Nonterminal.Base               LabeledItem
        [1].Seed.Expr.Concat.concat               ( Label ... ? Item
          [0].Seed.Expr.Postfix.?                 ( Label ':' ) ?
            [0].Seed.Expr.Parens.(                ( Label ':' )
              [0].Seed.Expr.Concat.concat         Label ':'
                [0].Seed.Nonterminal              Label
                  [0].Seed.Nonterminal.Base       Label
                [1].Seed.Terminal                 ':'
          [1].Seed.Nonterminal                    Item
            [0].Seed.Nonterminal.Base             Item
      [2].Seed.Rule                               Label = string
        [0].Seed.Nonterminal                      Label
          [0].Seed.Nonterminal.Base               Label
        [1].Seed.Terminal                         string
      [3].Seed.Rule                               Item = ... | number
        [0].Seed.Nonterminal                      Item
          [0].Seed.Nonterminal.Base               Item
        [1].Seed.Expr.Or.|                        ⊙ | string | number
          [0].Seed.Nonterminal                    ⊙
            [0].Seed.Nonterminal.Base             ⊙
          [1].Seed.Terminal                       string
          [2].Seed.Terminal                       number
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
