#include "seed.hpp"

#include "parse_root.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("seed-parse-root", "[seed][parse_root_t]")
{
  const tokenization_t seed_tokenization =
      SILVA_EXPECT_REQUIRE(tokenize(const_ptr_unowned(&seed_seed_source_code)));

  const parse_tree_t seed_pt_1 =
      SILVA_EXPECT_REQUIRE(seed_parse(const_ptr_unowned(&seed_tokenization)));
  const parse_tree_t seed_pt_2 =
      SILVA_EXPECT_REQUIRE(seed_parse_root()->apply(const_ptr_unowned(&seed_tokenization)));
  CHECK(seed_pt_1.nodes == seed_pt_2.nodes);
  CHECK(seed_pt_1.nodes == seed_parse_root()->seed_parse_tree->nodes);
}

TEST_CASE("seed", "[seed][parse_root_t]")
{
  const source_code_t sf_seed_source_code("simple-fern.seed", R"'(
    - SimpleFern = "[" ( LabeledItem ";"? )* "]"
    - LabeledItem = ( Label ":" )? Item
    - Label = string
    - Item,0 => SimpleFern,0
    - Item,1 =~ string number
  )'");
  const tokenization_t sf_seed_tokens =
      SILVA_EXPECT_REQUIRE(tokenize(const_ptr_unowned(&sf_seed_source_code)));

  const auto sf_seed_pt_1 = SILVA_EXPECT_REQUIRE(seed_parse(const_ptr_unowned(&sf_seed_tokens)));
  const auto sf_seed_pt_2 =
      SILVA_EXPECT_REQUIRE(seed_parse_root()->apply(const_ptr_unowned(&sf_seed_tokens)));
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

  const auto sfpr = SILVA_EXPECT_REQUIRE(parse_root_t::create(const_ptr_unowned(&sf_seed_pt_1)));
  REQUIRE(sfpr.rules.size() == 5);
  using rfl::json::write;
  CHECK(write(sfpr.rules[0]) ==
        R"({"token_id":1,"precedence":0,"name":"SimpleFern","expr_node_index":3})");
  CHECK(write(sfpr.rules[1]) ==
        R"({"token_id":5,"precedence":0,"name":"LabeledItem","expr_node_index":22})");
  CHECK(write(sfpr.rules[2]) ==
        R"({"token_id":11,"precedence":0,"name":"Label","expr_node_index":37})");
  CHECK(
      write(sfpr.rules[3]) ==
      R"({"token_id":13,"precedence":0,"name":"Item","expr_node_index":44,"aliased_rule_offset":0})");
  CHECK(write(sfpr.rules[4]) ==
        R"({"token_id":13,"precedence":1,"name":"Item","expr_node_index":50})");
  REQUIRE(sfpr.rule_indexes.size() == 4);
  REQUIRE(sfpr.rule_indexes.at(1) == 0);
  REQUIRE(sfpr.rule_indexes.at(5) == 1);
  REQUIRE(sfpr.rule_indexes.at(11) == 2);
  REQUIRE(sfpr.rule_indexes.at(13) == 3);

  const source_code_t sf_code("test.simple-fern", R"'( [ "abc" ; [ "def" 123 ] "jkl" ;])'");
  const tokenization_t sf_tokens = SILVA_EXPECT_REQUIRE(tokenize(const_ptr_unowned(&sf_code)));

  auto sfpt = SILVA_EXPECT_REQUIRE(sfpr.apply(const_ptr_unowned(&sf_tokens)));

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
