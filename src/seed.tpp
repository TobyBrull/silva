#include "seed.hpp"

#include "parse_root.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("seed-parse-root", "[seed][parse_root_t]")
{
  const tokenization_t seed_tokenization = tokenize(hybrid_ptr_const(&seed_seed_source_code));

  const parse_tree_t seed_pt_1 = SILVA_TRY_REQUIRE(seed_parse(&seed_tokenization));
  const parse_tree_t seed_pt_2 = SILVA_TRY_REQUIRE(seed_parse_root()->apply(&seed_tokenization));
  CHECK(seed_pt_1.nodes == seed_pt_2.nodes);
  CHECK(seed_pt_1.nodes == seed_parse_root()->seed_parse_tree.nodes);
}

TEST_CASE("seed", "[seed][parse_root_t]")
{
  const source_code_t sf_seed_source_code("simple-fern.seed", R"'(
    - SimpleFern = "[" ( LabeledItem ";"? )* "]"
    - LabeledItem = ( Label ":" )? Item
    - Label = string
    - Item,0 = SimpleFern
    - Item,1 = string
  )'");
  const tokenization_t sf_seed_tokens = tokenize(hybrid_ptr_const(&sf_seed_source_code));

  const auto sf_seed_pt_1 = SILVA_TRY_REQUIRE(seed_parse(&sf_seed_tokens));
  const auto sf_seed_pt_2 = SILVA_TRY_REQUIRE(seed_parse_root()->apply(&sf_seed_tokens));
  CHECK(sf_seed_pt_1.nodes == sf_seed_pt_2.nodes);

  const std::string_view expected = R"(
[.]Seed,0                                         -
  [0]Rule,0                                       SimpleFern
    [0]Nonterminal,0                              SimpleFern
    [1]Expr,1                                     "["
      [0]Atom,0                                   "["
        [0]Primary,1                              "["
          [0]Terminal,0                           "["
      [1]Atom,0                                   (
        [0]Primary,0                              (
          [0]Atom,0                               LabeledItem
            [0]Primary,2                          LabeledItem
              [0]Nonterminal,0                    LabeledItem
          [1]Atom,0                               ";"
            [0]Primary,1                          ";"
              [0]Terminal,0                       ";"
            [1]Suffix,0                           ?
        [1]Suffix,0                               *
      [2]Atom,0                                   "]"
        [0]Primary,1                              "]"
          [0]Terminal,0                           "]"
  [1]Rule,0                                       LabeledItem
    [0]Nonterminal,0                              LabeledItem
    [1]Expr,1                                     (
      [0]Atom,0                                   (
        [0]Primary,0                              (
          [0]Atom,0                               Label
            [0]Primary,2                          Label
              [0]Nonterminal,0                    Label
          [1]Atom,0                               ":"
            [0]Primary,1                          ":"
              [0]Terminal,0                       ":"
        [1]Suffix,0                               ?
      [1]Atom,0                                   Item
        [0]Primary,2                              Item
          [0]Nonterminal,0                        Item
  [2]Rule,0                                       Label
    [0]Nonterminal,0                              Label
    [1]Expr,1                                     string
      [0]Atom,0                                   string
        [0]Primary,1                              string
          [0]Terminal,0                           string
  [3]Rule,0                                       Item
    [0]Nonterminal,0                              Item
    [1]RulePrecedence,0                           0
    [2]Expr,1                                     SimpleFern
      [0]Atom,0                                   SimpleFern
        [0]Primary,2                              SimpleFern
          [0]Nonterminal,0                        SimpleFern
  [4]Rule,0                                       Item
    [0]Nonterminal,0                              Item
    [1]RulePrecedence,0                           1
    [2]Expr,1                                     string
      [0]Atom,0                                   string
        [0]Primary,1                              string
          [0]Terminal,0                           string
)";

  CHECK(parse_tree_to_string(sf_seed_pt_1) == expected.substr(1));
  CHECK(parse_tree_to_string(sf_seed_pt_2) == expected.substr(1));

  auto maybe_sf_parse_root = parse_root_t::create(sf_seed_pt_1);
  REQUIRE(maybe_sf_parse_root);
  auto sfpr = std::move(maybe_sf_parse_root).value();
  REQUIRE(sfpr.rules.size() == 5);
  using rfl::json::write;
  CHECK(write(sfpr.rules[0]) == R"({"name":"SimpleFern","precedence":0,"expr_node_index":3})");
  CHECK(write(sfpr.rules[1]) == R"({"name":"LabeledItem","precedence":0,"expr_node_index":22})");
  CHECK(write(sfpr.rules[2]) == R"({"name":"Label","precedence":0,"expr_node_index":37})");
  CHECK(write(sfpr.rules[3]) == R"({"name":"Item","precedence":0,"expr_node_index":44})");
  CHECK(write(sfpr.rules[4]) == R"({"name":"Item","precedence":1,"expr_node_index":51})");
  CHECK(
      write(sfpr.rule_name_offsets) ==
      R"([["Item",3],["Label",2],["LabeledItem",1],["SimpleFern",0]])");

  const source_code_t sf_code("test.simple-fern", R"'( [ "abc" ; [ "def" "ghi" ] "jkl" ;])'");
  const tokenization_t sf_tokens = tokenize(hybrid_ptr_const(&sf_code));

  auto sfpt = SILVA_TRY_REQUIRE(sfpr.apply(&sf_tokens));

  const std::string_view expected_parse_tree = R"(
[.]SimpleFern,0                                   [
  [0]LabeledItem,0                                "abc"
    [0]Item,1                                     "abc"
  [1]LabeledItem,0                                [
    [0]Item,0                                     [
      [0]SimpleFern,0                             [
        [0]LabeledItem,0                          "def"
          [0]Item,1                               "def"
        [1]LabeledItem,0                          "ghi"
          [0]Item,1                               "ghi"
  [2]LabeledItem,0                                "jkl"
    [0]Item,1                                     "jkl"
)";
  CHECK(parse_tree_to_string(sfpt) == expected_parse_tree.substr(1));
}
