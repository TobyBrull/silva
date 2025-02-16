#include "seed.hpp"

#include "parse_root.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("seed-parse-root", "[seed][parse_root_t]")
{
  token_context_t tc;
  const auto spr       = seed_parse_root(tc.ptr());
  const auto seed_tt   = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", string_t{seed_seed})));
  const auto seed_pt_1 = SILVA_EXPECT_REQUIRE(seed_parse(seed_tt));
  const auto seed_pt_2 = SILVA_EXPECT_REQUIRE(spr->apply(seed_tt));
  CHECK(seed_pt_1->nodes == seed_pt_2->nodes);
  CHECK(seed_pt_1->nodes == spr->seed_parse_tree->nodes);
}

TEST_CASE("seed", "[seed][parse_root_t]")
{
  token_context_t tc;
  const auto spr          = seed_parse_root(tc.ptr());
  const string_t sf_text  = R"'(
    - SimpleFern = "[" ( LabeledItem ";"? )* "]"
    - LabeledItem = ( Label ":" )? Item
    - Label = string
    - Item,0 => SimpleFern,0
    - Item,1 =~ string number
  )'";
  const auto sf_seed_tt   = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", sf_text)));
  const auto sf_seed_pt_1 = share(SILVA_EXPECT_REQUIRE(seed_parse(sf_seed_tt)));
  const auto sf_seed_pt_2 = SILVA_EXPECT_REQUIRE(spr->apply(sf_seed_tt));
  CHECK(sf_seed_pt_1->nodes == sf_seed_pt_2->nodes);

  const std::string_view expected = R"(
[0].Seed.0                                        - SimpleFern = ...
  [0].Rule.0                                      SimpleFern = "[" ...
    [0].Nonterminal.0                             SimpleFern
    [1].Derivation.0                              = "[" ( ...
      [0].Atom.1                                  "["
        [0].Primary.1                             "["
          [0].Terminal.1                          "["
      [1].Atom.1                                  ( LabeledItem ";" ...
        [0].Primary.0                             ( LabeledItem ";" ...
          [0].Atom.1                              LabeledItem
            [0].Primary.2                         LabeledItem
              [0].Nonterminal.0                   LabeledItem
          [1].Atom.1                              ";" ?
            [0].Primary.1                         ";"
              [0].Terminal.1                      ";"
            [1].Suffix.0                          ?
        [1].Suffix.0                              *
      [2].Atom.1                                  "]"
        [0].Primary.1                             "]"
          [0].Terminal.1                          "]"
  [1].Rule.0                                      LabeledItem = ( ...
    [0].Nonterminal.0                             LabeledItem
    [1].Derivation.0                              = ( Label ...
      [0].Atom.1                                  ( Label ":" ...
        [0].Primary.0                             ( Label ":" ...
          [0].Atom.1                              Label
            [0].Primary.2                         Label
              [0].Nonterminal.0                   Label
          [1].Atom.1                              ":"
            [0].Primary.1                         ":"
              [0].Terminal.1                      ":"
        [1].Suffix.0                              ?
      [1].Atom.1                                  Item
        [0].Primary.2                             Item
          [0].Nonterminal.0                       Item
  [2].Rule.0                                      Label = string
    [0].Nonterminal.0                             Label
    [1].Derivation.0                              = string
      [0].Atom.1                                  string
        [0].Primary.1                             string
          [0].Terminal.1                          string
  [3].Rule.0                                      Item , 0 ...
    [0].Nonterminal.0                             Item
    [1].RulePrecedence.0                          0
    [2].Derivation.2                              => SimpleFern , ...
      [0].Nonterminal.0                           SimpleFern
      [1].RulePrecedence.0                        0
  [4].Rule.0                                      Item , 1 ...
    [0].Nonterminal.0                             Item
    [1].RulePrecedence.0                          1
    [2].Derivation.1                              =~ string number
      [0].Terminal.1                              string
      [1].Terminal.1                              number
)";

  const string_t pt_str_1 = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sf_seed_pt_1));
  const string_t pt_str_2 = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sf_seed_pt_2));
  CHECK(pt_str_1 == expected.substr(1));
  CHECK(pt_str_2 == expected.substr(1));

  const auto sfpr = SILVA_EXPECT_REQUIRE(parse_root_t::create(sf_seed_pt_1));
  REQUIRE(sfpr->rules.size() == 5);
  using rfl::json::write;
  CHECK(sfpr->rules[0].precedence == 0);
  CHECK(sfpr->rules[0].rule_name == tc.full_name_id_of("SimpleFern", "0"));
  CHECK(sfpr->rules[0].expr_node_index == 3);
  CHECK(sfpr->rules[0].aliased_rule_offset.has_value() == false);
  CHECK(sfpr->rules[1].precedence == 0);
  CHECK(sfpr->rules[1].rule_name == tc.full_name_id_of("LabeledItem", "0"));
  CHECK(sfpr->rules[1].expr_node_index == 22);
  CHECK(sfpr->rules[1].aliased_rule_offset.has_value() == false);
  CHECK(sfpr->rules[2].precedence == 0);
  CHECK(sfpr->rules[2].rule_name == tc.full_name_id_of("Label", "0"));
  CHECK(sfpr->rules[2].expr_node_index == 37);
  CHECK(sfpr->rules[2].aliased_rule_offset.has_value() == false);
  CHECK(sfpr->rules[3].precedence == 0);
  CHECK(sfpr->rules[3].rule_name == tc.full_name_id_of("Item", "0"));
  CHECK(sfpr->rules[3].expr_node_index == 44);
  CHECK(sfpr->rules[3].aliased_rule_offset == 0);
  CHECK(sfpr->rules[4].precedence == 1);
  CHECK(sfpr->rules[4].rule_name == tc.full_name_id_of("Item", "1"));
  CHECK(sfpr->rules[4].expr_node_index == 50);
  CHECK(sfpr->rules[4].aliased_rule_offset.has_value() == false);
  REQUIRE(sfpr->rule_indexes.size() == 4);
  REQUIRE(sfpr->rule_indexes.at(tc.token_id("SimpleFern")) == 0);
  REQUIRE(sfpr->rule_indexes.at(tc.token_id("LabeledItem")) == 1);
  REQUIRE(sfpr->rule_indexes.at(tc.token_id("Label")) == 2);
  REQUIRE(sfpr->rule_indexes.at(tc.token_id("Item")) == 3);

  const string_t sf_code = R"'( [ "abc" ; [ "def" 123 ] "jkl" ;])'";
  const auto sf_tt       = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", sf_code)));
  const auto sfpt        = SILVA_EXPECT_REQUIRE(sfpr->apply(sf_tt));

  const std::string_view expected_parse_tree = R"(
[0].SimpleFern.0                                  [ "abc" ; ...
  [0].LabeledItem.0                               "abc"
    [0].Item.1                                    "abc"
  [1].LabeledItem.0                               [ "def" 123 ...
    [0].SimpleFern.0                              [ "def" 123 ...
      [0].LabeledItem.0                           "def"
        [0].Item.1                                "def"
      [1].LabeledItem.0                           123
        [0].Item.1                                123
  [2].LabeledItem.0                               "jkl"
    [0].Item.1                                    "jkl"
)";
  const string_t result{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sfpt))};
  CHECK(result == expected_parse_tree.substr(1));
}
