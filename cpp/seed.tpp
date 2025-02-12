#include "seed.hpp"

#include "parse_root.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("seed-parse-root", "[seed][parse_root_t]")
{
  const tokenization_t tt      = SILVA_EXPECT_REQUIRE(token_context_make("", string_t{seed_seed}));
  const parse_tree_t seed_pt_1 = SILVA_EXPECT_REQUIRE(seed_parse(tt.ptr()));
  const parse_tree_t seed_pt_2 = SILVA_EXPECT_REQUIRE(seed_parse_root()->apply(tt.ptr()));
  CHECK(seed_pt_1.nodes == seed_pt_2.nodes);
  CHECK(seed_pt_1.nodes == seed_parse_root()->seed_parse_tree->nodes);
}

TEST_CASE("seed", "[seed][parse_root_t]")
{
  const string_t sf_text              = R"'(
    - SimpleFern = "[" ( LabeledItem ";"? )* "]"
    - LabeledItem = ( Label ":" )? Item
    - Label = string
    - Item,0 => SimpleFern,0
    - Item,1 =~ string number
  )'";
  const tokenization_t sf_seed_tokens = SILVA_EXPECT_REQUIRE(token_context_make("", sf_text));
  const auto sf_seed_pt_1             = SILVA_EXPECT_REQUIRE(seed_parse(sf_seed_tokens.ptr()));
  const auto sf_seed_pt_2 = SILVA_EXPECT_REQUIRE(seed_parse_root()->apply(sf_seed_tokens.ptr()));
  CHECK(sf_seed_pt_1.nodes == sf_seed_pt_2.nodes);

  const std::string_view expected = R"(
[0]Seed,0                                         -
  [0]Rule,0                                       SimpleFern
    [0]Nonterminal,0                              SimpleFern
    [1]Derivation,0                               =
      [0]Atom,1                                   "["
        [0]Primary,1                              "["
          [0]Terminal,1                           "["
      [1]Atom,1                                   (
        [0]Primary,0                              (
          [0]Atom,1                               LabeledItem
            [0]Primary,2                          LabeledItem
              [0]Nonterminal,0                    LabeledItem
          [1]Atom,1                               ";"
            [0]Primary,1                          ";"
              [0]Terminal,1                       ";"
            [1]Suffix,0                           ?
        [1]Suffix,0                               *
      [2]Atom,1                                   "]"
        [0]Primary,1                              "]"
          [0]Terminal,1                           "]"
  [1]Rule,0                                       LabeledItem
    [0]Nonterminal,0                              LabeledItem
    [1]Derivation,0                               =
      [0]Atom,1                                   (
        [0]Primary,0                              (
          [0]Atom,1                               Label
            [0]Primary,2                          Label
              [0]Nonterminal,0                    Label
          [1]Atom,1                               ":"
            [0]Primary,1                          ":"
              [0]Terminal,1                       ":"
        [1]Suffix,0                               ?
      [1]Atom,1                                   Item
        [0]Primary,2                              Item
          [0]Nonterminal,0                        Item
  [2]Rule,0                                       Label
    [0]Nonterminal,0                              Label
    [1]Derivation,0                               =
      [0]Atom,1                                   string
        [0]Primary,1                              string
          [0]Terminal,1                           string
  [3]Rule,0                                       Item
    [0]Nonterminal,0                              Item
    [1]RulePrecedence,0                           0
    [2]Derivation,2                               =>
      [0]Nonterminal,0                            SimpleFern
      [1]RulePrecedence,0                         0
  [4]Rule,0                                       Item
    [0]Nonterminal,0                              Item
    [1]RulePrecedence,0                           1
    [2]Derivation,1                               =~
      [0]Terminal,1                               string
      [1]Terminal,1                               number
)";

  const string_t pt_str_1 = SILVA_EXPECT_REQUIRE(parse_tree_to_string(sf_seed_pt_1));
  const string_t pt_str_2 = SILVA_EXPECT_REQUIRE(parse_tree_to_string(sf_seed_pt_2));
  CHECK(pt_str_1 == expected.substr(1));
  CHECK(pt_str_2 == expected.substr(1));

  const auto sfpr = SILVA_EXPECT_REQUIRE(parse_root_t::create(sf_seed_pt_1.ptr()));
  REQUIRE(sfpr.rules.size() == 5);
  using rfl::json::write;
  CHECK(sfpr.rules[0].precedence == 0);
  CHECK(sfpr.rules[0].name == "SimpleFern");
  CHECK(sfpr.rules[0].expr_node_index == 3);
  CHECK(sfpr.rules[0].aliased_rule_offset.has_value() == false);
  CHECK(sfpr.rules[1].precedence == 0);
  CHECK(sfpr.rules[1].name == "LabeledItem");
  CHECK(sfpr.rules[1].expr_node_index == 22);
  CHECK(sfpr.rules[1].aliased_rule_offset.has_value() == false);
  CHECK(sfpr.rules[2].precedence == 0);
  CHECK(sfpr.rules[2].name == "Label");
  CHECK(sfpr.rules[2].expr_node_index == 37);
  CHECK(sfpr.rules[2].aliased_rule_offset.has_value() == false);
  CHECK(sfpr.rules[3].precedence == 0);
  CHECK(sfpr.rules[3].name == "Item");
  CHECK(sfpr.rules[3].expr_node_index == 44);
  CHECK(sfpr.rules[3].aliased_rule_offset == 0);
  CHECK(sfpr.rules[4].precedence == 1);
  CHECK(sfpr.rules[4].name == "Item");
  CHECK(sfpr.rules[4].expr_node_index == 50);
  CHECK(sfpr.rules[4].aliased_rule_offset.has_value() == false);
  REQUIRE(sfpr.rule_indexes.size() == 4);
  REQUIRE(sfpr.rule_indexes.at(token_context_get_index("SimpleFern")) == 0);
  REQUIRE(sfpr.rule_indexes.at(token_context_get_index("LabeledItem")) == 1);
  REQUIRE(sfpr.rule_indexes.at(token_context_get_index("Label")) == 2);
  REQUIRE(sfpr.rule_indexes.at(token_context_get_index("Item")) == 3);

  const string_t sf_code         = R"'( [ "abc" ; [ "def" 123 ] "jkl" ;])'";
  const tokenization_t sf_tokens = SILVA_EXPECT_REQUIRE(token_context_make("", sf_code));
  auto sfpt                      = SILVA_EXPECT_REQUIRE(sfpr.apply(sf_tokens.ptr()));

  const std::string_view expected_parse_tree = R"(
[0]SimpleFern,0                                   [
  [0]LabeledItem,0                                "abc"
    [0]Item,1                                     "abc"
  [1]LabeledItem,0                                [
    [0]SimpleFern,0                               [
      [0]LabeledItem,0                            "def"
        [0]Item,1                                 "def"
      [1]LabeledItem,0                            123
        [0]Item,1                                 123
  [2]LabeledItem,0                                "jkl"
    [0]Item,1                                     "jkl"
)";
  const string_t result{SILVA_EXPECT_REQUIRE(parse_tree_to_string(sfpt))};
  CHECK(result == expected_parse_tree.substr(1));
}
