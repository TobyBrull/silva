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
[0]Silva.Seed                                     - SimpleFern ... | number
  [0]Silva.Rule                                   SimpleFern = ... * ']'
    [0]Silva.Nonterminal                          SimpleFern
    [1]Silva.Expr.Concat.concat                   '[' ( ... * ']'
      [0]Silva.Terminal                           '['
      [1]Silva.Expr.Postfix.*                     ( LabeledItem ... ) *
        [0]Silva.Expr.Parens.(                    ( LabeledItem ';' ? )
          [0]Silva.Expr.Concat.concat             LabeledItem ';' ?
            [0]Silva.Nonterminal                  LabeledItem
            [1]Silva.Expr.Postfix.?               ';' ?
              [0]Silva.Terminal                   ';'
      [2]Silva.Terminal                           ']'
  [1]Silva.Rule                                   LabeledItem = ... ? Item
    [0]Silva.Nonterminal                          LabeledItem
    [1]Silva.Expr.Concat.concat                   ( Label ... ? Item
      [0]Silva.Expr.Postfix.?                     ( Label ':' ) ?
        [0]Silva.Expr.Parens.(                    ( Label ':' )
          [0]Silva.Expr.Concat.concat             Label ':'
            [0]Silva.Nonterminal                  Label
            [1]Silva.Terminal                     ':'
      [1]Silva.Nonterminal                        Item
  [2]Silva.Rule                                   Label = string
    [0]Silva.Nonterminal                          Label
    [1]Silva.Terminal                             string
  [3]Silva.Rule                                   Item = ... | number
    [0]Silva.Nonterminal                          Item
    [1]Silva.Expr.Alt.|                           SimpleFern | string | number
      [0]Silva.Nonterminal                        SimpleFern
      [1]Silva.Terminal                           string
      [2]Silva.Terminal                           number
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
[0]Silva.SimpleFern                               [ 'abc' ... ; ]
  [0]Silva.LabeledItem                            'abc'
    [0]Silva.Item                                 'abc'
  [1]Silva.LabeledItem                            [ 'def' 123 ]
    [0]Silva.Item                                 [ 'def' 123 ]
      [0]Silva.SimpleFern                         [ 'def' 123 ]
        [0]Silva.LabeledItem                      'def'
          [0]Silva.Item                           'def'
        [1]Silva.LabeledItem                      123
          [0]Silva.Item                           123
  [2]Silva.LabeledItem                            'jkl'
    [0]Silva.Item                                 'jkl'
)";
  const string_t result{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sfpt))};
  CHECK(result == expected_parse_tree.substr(1));
}
