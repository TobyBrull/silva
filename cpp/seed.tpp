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
  const string_t sf_text  = R"'(
    - SimpleFern = '[' ( LabeledItem ';' ? ) * ']'
    - LabeledItem = ( Label ':' )? Item
    - Label = string
    - Item = SimpleFern | string | number
  )'";
  const auto sf_seed_tt   = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", sf_text)));
  const auto sf_seed_pt_1 = share(SILVA_EXPECT_REQUIRE(seed_parse(sf_seed_tt)));
  const auto spr          = seed_parse_root(tc.ptr());
  const auto sf_seed_pt_2 = SILVA_EXPECT_REQUIRE(spr->apply(sf_seed_tt));
  CHECK(sf_seed_pt_1->nodes == sf_seed_pt_2->nodes);

  const std::string_view expected = R"(
[0]~Seed                                          - SimpleFern = ...
  [0]~Rule                                        SimpleFern = '[' ...
    [0]~Nonterminal                               SimpleFern
    [1]~Expr~Concat~                              '[' ( LabeledItem ...
      [0]~Terminal                                '['
      [1]~Expr~Postfix~*                          ( LabeledItem ';' ...
        [0]~Expr~Parens~(                         ( LabeledItem ';' ...
          [0]~Expr~Concat~                        LabeledItem ';' ?
            [0]~Nonterminal                       LabeledItem
            [1]~Expr~Postfix~?                    ';' ?
              [0]~Terminal                        ';'
      [2]~Terminal                                ']'
  [1]~Rule                                        LabeledItem = ( ...
    [0]~Nonterminal                               LabeledItem
    [1]~Expr~Concat~                              ( Label ':' ...
      [0]~Expr~Postfix~?                          ( Label ':' ...
        [0]~Expr~Parens~(                         ( Label ':' ...
          [0]~Expr~Concat~                        Label ':'
            [0]~Nonterminal                       Label
            [1]~Terminal                          ':'
      [1]~Nonterminal                             Item
  [2]~Rule                                        Label = string
    [0]~Nonterminal                               Label
    [1]~Terminal                                  string
  [3]~Rule                                        Item = SimpleFern ...
    [0]~Nonterminal                               Item
    [1]~Expr~Alt~|                                SimpleFern | string ...
      [0]~Nonterminal                             SimpleFern
      [1]~Terminal                                string
      [2]~Terminal                                number
)";

  const string_t pt_str_1 = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sf_seed_pt_1));
  const string_t pt_str_2 = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sf_seed_pt_2));
  CHECK(pt_str_1 == expected.substr(1));
  CHECK(pt_str_2 == expected.substr(1));

  const auto sfpr = SILVA_EXPECT_REQUIRE(parse_root_t::create(sf_seed_pt_1));
  REQUIRE(sfpr->rules.size() == 4);
  using rfl::json::write;
  CHECK(sfpr->rules[0].name == tc.full_name_id_of("SimpleFern"));
  CHECK(sfpr->rules[0].expr_node_index == 3);
  CHECK(sfpr->rules[1].name == tc.full_name_id_of("LabeledItem"));
  CHECK(sfpr->rules[1].expr_node_index == 14);
  CHECK(sfpr->rules[2].name == tc.full_name_id_of("Label"));
  CHECK(sfpr->rules[2].expr_node_index == 23);
  CHECK(sfpr->rules[3].name == tc.full_name_id_of("Item"));
  CHECK(sfpr->rules[3].expr_node_index == 26);
  REQUIRE(sfpr->rule_indexes.size() == 4);
  REQUIRE(sfpr->rule_indexes.at(tc.full_name_id_of("SimpleFern")) == 0);
  REQUIRE(sfpr->rule_indexes.at(tc.full_name_id_of("LabeledItem")) == 1);
  REQUIRE(sfpr->rule_indexes.at(tc.full_name_id_of("Label")) == 2);
  REQUIRE(sfpr->rule_indexes.at(tc.full_name_id_of("Item")) == 3);

  const string_t sf_code = R"'( [ 'abc' ; [ 'def' 123 ] 'jkl' ;])'";
  const auto sf_tt       = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", sf_code)));
  const auto sfpt        = SILVA_EXPECT_REQUIRE(sfpr->apply(sf_tt));

  const std::string_view expected_parse_tree = R"(
[0]~SimpleFern                                    [ 'abc' ; ...
  [0]~LabeledItem                                 'abc'
    [0]~Item                                      'abc'
  [1]~LabeledItem                                 [ 'def' 123 ...
    [0]~Item                                      [ 'def' 123 ...
      [0]~SimpleFern                              [ 'def' 123 ...
        [0]~LabeledItem                           'def'
          [0]~Item                                'def'
        [1]~LabeledItem                           123
          [0]~Item                                123
  [2]~LabeledItem                                 'jkl'
    [0]~Item                                      'jkl'
)";
  const string_t result{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sfpt))};
  CHECK(result == expected_parse_tree.substr(1));
}
