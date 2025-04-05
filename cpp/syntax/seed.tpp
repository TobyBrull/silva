#include "seed.hpp"

#include "seed_engine.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("seed-parse-root", "[seed][seed_engine_t]")
{
  token_context_t tc;
  const auto spr       = seed_seed_engine(tc.ptr());
  const auto seed_tt   = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", string_t{seed_seed})));
  const auto seed_pt_1 = SILVA_EXPECT_REQUIRE(seed_parse(seed_tt));
  const auto seed_pt_2 = SILVA_EXPECT_REQUIRE(spr->apply(seed_tt, tc.name_id_of("Seed")));
  CHECK(seed_pt_1->nodes == seed_pt_2->nodes);
  REQUIRE(spr->seed_parse_trees.size() == 1);
  CHECK(seed_pt_1->nodes == spr->seed_parse_trees.front()->nodes);

  CHECK(spr->keyword_scopes[tc.name_id_of("Seed", "Rule")] == hashset_t<token_id_t>({}));
  CHECK(spr->keyword_scopes[tc.name_id_of("Seed", "Axe")] ==
        hashset_t<token_id_t>({
            *tc.token_id("=/"),
            *tc.token_id("["),
            *tc.token_id("]"),
            *tc.token_id("-"),
            *tc.token_id("="),
            *tc.token_id("nest"),
            *tc.token_id("ltr"),
            *tc.token_id("rtl"),
            *tc.token_id("atom_nest"),
            *tc.token_id("prefix"),
            *tc.token_id("prefix_nest"),
            *tc.token_id("infix"),
            *tc.token_id("infix_flat"),
            *tc.token_id("ternary"),
            *tc.token_id("postfix"),
            *tc.token_id("postfix_nest"),
            *tc.token_id("concat"),
        }));
}

TEST_CASE("seed", "[seed][seed_engine_t]")
{
  const string_t sf_text = R"'(
    - SimpleFern = [
      - X = '[' ( LabeledItem ';' ? ) * ']'
      - LabeledItem = ( Label ':' )? Item
      - Label = string
      - Item = X | string | number
    ]
  )'";
  token_context_t tc;
  const auto sf_seed_tt   = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", sf_text)));
  const auto sf_seed_pt_1 = share(SILVA_EXPECT_REQUIRE(seed_parse(sf_seed_tt)));
  const auto spr          = seed_seed_engine(tc.ptr());
  const auto sf_seed_pt_2 = SILVA_EXPECT_REQUIRE(spr->apply(sf_seed_tt, tc.name_id_of("Seed")));
  CHECK(sf_seed_pt_1->nodes == sf_seed_pt_2->nodes);

  const std::string_view expected = R"(
[0]Silva.Seed                                     - SimpleFern ... number ]
  [0]Silva.Seed.Rule                              SimpleFern = ... number ]
    [0]Silva.Seed.Nonterminal.Base                SimpleFern
    [1]Silva.Seed                                 - X ... | number
      [0]Silva.Seed.Rule                          X = ... * ']'
        [0]Silva.Seed.Nonterminal.Base            X
        [1]Silva.Seed.ExprOrAlias                 = '[' ... * ']'
          [0]Silva.Seed.Expr.Concat.concat        '[' ( ... * ']'
            [0]Silva.Seed.Terminal                '['
            [1]Silva.Seed.Expr.Postfix.*          ( LabeledItem ... ) *
              [0]Silva.Seed.Expr.Parens.(         ( LabeledItem ';' ? )
                [0]Silva.Seed.Expr.Concat.concat  LabeledItem ';' ?
                  [0]Silva.Seed.Nonterminal       LabeledItem
                    [0]Silva.Seed.Nonterminal.Base LabeledItem
                  [1]Silva.Seed.Expr.Postfix.?    ';' ?
                    [0]Silva.Seed.Terminal        ';'
            [2]Silva.Seed.Terminal                ']'
      [1]Silva.Seed.Rule                          LabeledItem = ... ? Item
        [0]Silva.Seed.Nonterminal.Base            LabeledItem
        [1]Silva.Seed.ExprOrAlias                 = ( ... ? Item
          [0]Silva.Seed.Expr.Concat.concat        ( Label ... ? Item
            [0]Silva.Seed.Expr.Postfix.?          ( Label ':' ) ?
              [0]Silva.Seed.Expr.Parens.(         ( Label ':' )
                [0]Silva.Seed.Expr.Concat.concat  Label ':'
                  [0]Silva.Seed.Nonterminal       Label
                    [0]Silva.Seed.Nonterminal.Base Label
                  [1]Silva.Seed.Terminal          ':'
            [1]Silva.Seed.Nonterminal             Item
              [0]Silva.Seed.Nonterminal.Base      Item
      [2]Silva.Seed.Rule                          Label = string
        [0]Silva.Seed.Nonterminal.Base            Label
        [1]Silva.Seed.ExprOrAlias                 = string
          [0]Silva.Seed.Terminal                  string
      [3]Silva.Seed.Rule                          Item = ... | number
        [0]Silva.Seed.Nonterminal.Base            Item
        [1]Silva.Seed.ExprOrAlias                 = X ... | number
          [0]Silva.Seed.Expr.Or.|                 X | string | number
            [0]Silva.Seed.Nonterminal             X
              [0]Silva.Seed.Nonterminal.Base      X
            [1]Silva.Seed.Terminal                string
            [2]Silva.Seed.Terminal                number
)";

  const string_t pt_str_1 = SILVA_EXPECT_REQUIRE(sf_seed_pt_1->span().to_string());
  const string_t pt_str_2 = SILVA_EXPECT_REQUIRE(sf_seed_pt_2->span().to_string());
  CHECK(pt_str_1 == expected.substr(1));
  CHECK(pt_str_2 == expected.substr(1));

  seed_engine_t se(tc.ptr());
  SILVA_EXPECT_REQUIRE(se.add_complete(sf_seed_pt_1));
  REQUIRE(se.rule_exprs.size() == 4);
  using rfl::json::write;
  const name_id_t fni_sf      = tc.name_id_of("SimpleFern");
  const name_id_t fni_li      = tc.name_id_of(fni_sf, "LabeledItem");
  const name_id_t fni_l       = tc.name_id_of(fni_sf, "Label");
  const name_id_t fni_i       = tc.name_id_of(fni_sf, "Item");
  const parse_tree_span_t pts = se.seed_parse_trees.front()->span();
  CHECK(se.rule_exprs.at(fni_sf) == pts.sub_tree_span_at(6));
  CHECK(se.rule_exprs.at(fni_li) == pts.sub_tree_span_at(19));
  CHECK(se.rule_exprs.at(fni_l) == pts.sub_tree_span_at(31));
  CHECK(se.rule_exprs.at(fni_i) == pts.sub_tree_span_at(35));
  REQUIRE(se.nonterminal_rules.size() == 4);
  CHECK(se.nonterminal_rules.at(pts.sub_tree_span_at(12)) == fni_li);
  CHECK(se.nonterminal_rules.at(pts.sub_tree_span_at(24)) == fni_l);
  CHECK(se.nonterminal_rules.at(pts.sub_tree_span_at(27)) == fni_i);
  CHECK(se.nonterminal_rules.at(pts.sub_tree_span_at(37)) == fni_sf);

  const string_t sf_code = R"'( [ 'abc' ; [ 'def' 123 ] 'jkl' ;])'";
  const auto sf_tt       = share(SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "", sf_code)));
  const auto sfpt        = SILVA_EXPECT_REQUIRE(se.apply(sf_tt, fni_sf));

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
  const string_t result{SILVA_EXPECT_REQUIRE(sfpt->span().to_string())};
  CHECK(result == expected_parse_tree.substr(1));
}
