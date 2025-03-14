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
  const auto seed_pt_2 = SILVA_EXPECT_REQUIRE(spr->apply(seed_tt, tc.full_name_id_of("Seed")));
  CHECK(seed_pt_1->nodes == seed_pt_2->nodes);
  REQUIRE(spr->seed_parse_trees.size() == 1);
  CHECK(seed_pt_1->nodes == spr->seed_parse_trees.front()->nodes);
}

TEST_CASE("seed", "[seed][parse_root_t]")
{
  token_context_t tc;
  const string_t sf_text  = R"'(
    - SimpleFern [
      - X = '[' ( LabeledItem ';' ? ) * ']'
      - LabeledItem = ( Label ':' )? Item
      - Label = string
      - Item = X | string | number
    ]
  )'";
  const auto sf_seed_tt   = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", sf_text)));
  const auto sf_seed_pt_1 = share(SILVA_EXPECT_REQUIRE(seed_parse(sf_seed_tt)));
  const auto spr          = seed_parse_root(tc.ptr());
  const auto sf_seed_pt_2 =
      SILVA_EXPECT_REQUIRE(spr->apply(sf_seed_tt, tc.full_name_id_of("Seed")));
  CHECK(sf_seed_pt_1->nodes == sf_seed_pt_2->nodes);

  const std::string_view expected = R"(
[0]Silva.Seed                                     - SimpleFern ... number ]
  [0]Silva.Seed.Rule                              SimpleFern [ ... number ]
    [0]Silva.Seed.Nonterminal.Base                SimpleFern
    [1]Silva.Seed                                 - X ... | number
      [0]Silva.Seed.Rule                          X = ... * ']'
        [0]Silva.Seed.Nonterminal.Base            X
        [1]Silva.Seed.Expr.Concat.concat          '[' ( ... * ']'
          [0]Silva.Seed.Terminal                  '['
          [1]Silva.Seed.Expr.Postfix.*            ( LabeledItem ... ) *
            [0]Silva.Seed.Expr.Parens.(           ( LabeledItem ';' ? )
              [0]Silva.Seed.Expr.Concat.concat    LabeledItem ';' ?
                [0]Silva.Seed.Nonterminal         LabeledItem
                  [0]Silva.Seed.Nonterminal.Base  LabeledItem
                [1]Silva.Seed.Expr.Postfix.?      ';' ?
                  [0]Silva.Seed.Terminal          ';'
          [2]Silva.Seed.Terminal                  ']'
      [1]Silva.Seed.Rule                          LabeledItem = ... ? Item
        [0]Silva.Seed.Nonterminal.Base            LabeledItem
        [1]Silva.Seed.Expr.Concat.concat          ( Label ... ? Item
          [0]Silva.Seed.Expr.Postfix.?            ( Label ':' ) ?
            [0]Silva.Seed.Expr.Parens.(           ( Label ':' )
              [0]Silva.Seed.Expr.Concat.concat    Label ':'
                [0]Silva.Seed.Nonterminal         Label
                  [0]Silva.Seed.Nonterminal.Base  Label
                [1]Silva.Seed.Terminal            ':'
          [1]Silva.Seed.Nonterminal               Item
            [0]Silva.Seed.Nonterminal.Base        Item
      [2]Silva.Seed.Rule                          Label = string
        [0]Silva.Seed.Nonterminal.Base            Label
        [1]Silva.Seed.Terminal                    string
      [3]Silva.Seed.Rule                          Item = ... | number
        [0]Silva.Seed.Nonterminal.Base            Item
        [1]Silva.Seed.Expr.Alt.|                  X | string | number
          [0]Silva.Seed.Nonterminal               X
            [0]Silva.Seed.Nonterminal.Base        X
          [1]Silva.Seed.Terminal                  string
          [2]Silva.Seed.Terminal                  number
)";

  const string_t pt_str_1 = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sf_seed_pt_1));
  const string_t pt_str_2 = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sf_seed_pt_2));
  CHECK(pt_str_1 == expected.substr(1));
  CHECK(pt_str_2 == expected.substr(1));

  const auto sfpr = SILVA_EXPECT_REQUIRE(parse_root_t::create(sf_seed_pt_1));
  REQUIRE(sfpr->rule_exprs.size() == 4);
  using rfl::json::write;
  using tni_t                 = parse_root_t::tree_node_index_t;
  const full_name_id_t fni_sf = tc.full_name_id_of("SimpleFern");
  const full_name_id_t fni_li = tc.full_name_id_of(fni_sf, "LabeledItem");
  const full_name_id_t fni_l  = tc.full_name_id_of(fni_sf, "Label");
  const full_name_id_t fni_i  = tc.full_name_id_of(fni_sf, "Item");
  CHECK(sfpr->rule_exprs.at(fni_sf) == tni_t{.node_index = 6});
  CHECK(sfpr->rule_exprs.at(fni_li) == tni_t{.node_index = 18});
  CHECK(sfpr->rule_exprs.at(fni_l) == tni_t{.node_index = 29});
  CHECK(sfpr->rule_exprs.at(fni_i) == tni_t{.node_index = 32});
  REQUIRE(sfpr->nonterminal_rules.size() == 4);
  REQUIRE(sfpr->nonterminal_rules.at(tni_t{.node_index = 11}) == fni_li);
  REQUIRE(sfpr->nonterminal_rules.at(tni_t{.node_index = 22}) == fni_l);
  REQUIRE(sfpr->nonterminal_rules.at(tni_t{.node_index = 25}) == fni_i);
  REQUIRE(sfpr->nonterminal_rules.at(tni_t{.node_index = 33}) == fni_sf);

  const string_t sf_code = R"'( [ 'abc' ; [ 'def' 123 ] 'jkl' ;])'";
  const auto sf_tt       = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", sf_code)));
  const auto sfpt        = SILVA_EXPECT_REQUIRE(sfpr->apply(sf_tt, fni_sf));

  const std::string_view expected_parse_tree = R"(
[0]Silva.SimpleFern                               [ 'abc' ... ; ]
  [0]Silva.SimpleFern.LabeledItem                 'abc'
    [0]Silva.SimpleFern.Item                      'abc'
  [1]Silva.SimpleFern.LabeledItem                 [ 'def' 123 ]
    [0]Silva.SimpleFern.Item                      [ 'def' 123 ]
      [0]Silva.SimpleFern                         [ 'def' 123 ]
        [0]Silva.SimpleFern.LabeledItem           'def'
          [0]Silva.SimpleFern.Item                'def'
        [1]Silva.SimpleFern.LabeledItem           123
          [0]Silva.SimpleFern.Item                123
  [2]Silva.SimpleFern.LabeledItem                 'jkl'
    [0]Silva.SimpleFern.Item                      'jkl'
)";
  const string_t result{SILVA_EXPECT_REQUIRE(parse_tree_to_string(*sfpt))};
  CHECK(result == expected_parse_tree.substr(1));
}
