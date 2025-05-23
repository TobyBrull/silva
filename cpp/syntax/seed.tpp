#include "seed.hpp"

#include "name_id_style.hpp"
#include "syntax.hpp"

#include "rfl/json/write.hpp"

#include <catch2/catch_all.hpp>

namespace silva::seed::test {
  TEST_CASE("name-id-style", "[name_id_style_t]")
  {
    syntax_ward_t sw;

    const name_id_t name1 = sw.name_id_of("std", "expr", "stmt");
    const name_id_t name2 = sw.name_id_of("std", "expr");
    const name_id_t name3 = sw.name_id_of("std", "ranges", "vector");

    const name_id_style_t ts{
        .swp       = sw.ptr(),
        .root      = *sw.token_id("cpp"),
        .current   = *sw.token_id("this"),
        .parent    = *sw.token_id("super"),
        .separator = *sw.token_id("::"),
    };
    CHECK(ts.absolute(name1) == "cpp::std::expr::stmt");
    CHECK(ts.absolute(name2) == "cpp::std::expr");
    CHECK(ts.absolute(name3) == "cpp::std::ranges::vector");
    CHECK(ts.relative(name1, name1) == "this");
    CHECK(ts.relative(name2, name1) == "stmt");
    CHECK(ts.relative(name3, name1) == "super::super::expr::stmt");
    CHECK(ts.relative(name1, name2) == "super");
    CHECK(ts.relative(name2, name2) == "this");
    CHECK(ts.relative(name3, name2) == "super::super::expr");
    CHECK(ts.relative(name1, name3) == "super::super::ranges::vector");
    CHECK(ts.relative(name2, name3) == "super::ranges::vector");
    CHECK(ts.relative(name3, name3) == "this");
  }

  TEST_CASE("seed-parse-root", "[seed][seed_engine_t]")
  {
    syntax_ward_t sw;
    const auto spr       = standard_seed_engine(sw.ptr());
    const auto seed_tt   = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "", string_t{seed_str}));
    const auto seed_pt_1 = SILVA_EXPECT_REQUIRE(seed_parse(seed_tt));
    const auto seed_pt_2 = SILVA_EXPECT_REQUIRE(spr->apply(seed_tt, sw.name_id_of("Seed")));
    // fmt::print("|{}|\n", *seed_pt_1->span().to_string());
    // fmt::print("|{}|\n", *seed_pt_2->span().to_string());
    CHECK(seed_pt_1->nodes == seed_pt_2->nodes);

    CHECK(spr->keyword_scopes[sw.name_id_of("Seed", "Rule")] == hashset_t<token_id_t>({}));
    CHECK(spr->keyword_scopes[sw.name_id_of("Seed", "Axe")] ==
          hashset_t<token_id_t>({
              *sw.token_id("["),
              *sw.token_id("]"),
              *sw.token_id("-"),
              *sw.token_id("="),
              *sw.token_id("nest"),
              *sw.token_id("ltr"),
              *sw.token_id("rtl"),
              *sw.token_id("atom_nest"),
              *sw.token_id("atom_nest_transparent"),
              *sw.token_id("prefix"),
              *sw.token_id("prefix_nest"),
              *sw.token_id("infix"),
              *sw.token_id("infix_flat"),
              *sw.token_id("ternary"),
              *sw.token_id("postfix"),
              *sw.token_id("postfix_nest"),
              *sw.token_id("concat"),
              *sw.token_id("->"),
          }));
  }

  TEST_CASE("seed", "[seed][seed_engine_t]")
  {
    const string_t sf_text = R"'(
    - SimpleFern = [
      - x = '[' ( LabeledItem ';' ? ) * ']'
      - LabeledItem = ( Label ':' )? Item
      - Label = string
      - Item = x | string | number
    ]
  )'";
    syntax_ward_t sw;
    const auto sf_seed_tt   = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "", sf_text));
    const auto sf_seed_pt_1 = SILVA_EXPECT_REQUIRE(seed_parse(sf_seed_tt));
    const auto spr          = standard_seed_engine(sw.ptr());
    const auto sf_seed_pt_2 = SILVA_EXPECT_REQUIRE(spr->apply(sf_seed_tt, sw.name_id_of("Seed")));
    CHECK(sf_seed_pt_1->nodes == sf_seed_pt_2->nodes);

    const std::string_view expected = R"(
[0]_.Seed                                         - SimpleFern ... number ]
  [0]_.Seed.Rule                                  SimpleFern = ... number ]
    [0]_.Seed.Nonterminal                         SimpleFern
      [0]_.Seed.Nonterminal.Base                  SimpleFern
    [1]_.Seed                                     - x ... | number
      [0]_.Seed.Rule                              x = ... * ']'
        [0]_.Seed.Nonterminal                     x
          [0]_.Seed.Nonterminal.Base              x
        [1]_.Seed.ExprOrAlias                     = '[' ... * ']'
          [0]_.Seed.Expr.Concat.concat            '[' ( ... * ']'
            [0]_.Seed.Terminal                    '['
            [1]_.Seed.Expr.Postfix.*              ( LabeledItem ... ) *
              [0]_.Seed.Expr.Parens.(             ( LabeledItem ';' ? )
                [0]_.Seed.Expr.Concat.concat      LabeledItem ';' ?
                  [0]_.Seed.NonterminalMaybeVar   LabeledItem
                    [0]_.Seed.Nonterminal         LabeledItem
                      [0]_.Seed.Nonterminal.Base  LabeledItem
                  [1]_.Seed.Expr.Postfix.?        ';' ?
                    [0]_.Seed.Terminal            ';'
            [2]_.Seed.Terminal                    ']'
      [1]_.Seed.Rule                              LabeledItem = ... ? Item
        [0]_.Seed.Nonterminal                     LabeledItem
          [0]_.Seed.Nonterminal.Base              LabeledItem
        [1]_.Seed.ExprOrAlias                     = ( ... ? Item
          [0]_.Seed.Expr.Concat.concat            ( Label ... ? Item
            [0]_.Seed.Expr.Postfix.?              ( Label ':' ) ?
              [0]_.Seed.Expr.Parens.(             ( Label ':' )
                [0]_.Seed.Expr.Concat.concat      Label ':'
                  [0]_.Seed.NonterminalMaybeVar   Label
                    [0]_.Seed.Nonterminal         Label
                      [0]_.Seed.Nonterminal.Base  Label
                  [1]_.Seed.Terminal              ':'
            [1]_.Seed.NonterminalMaybeVar         Item
              [0]_.Seed.Nonterminal               Item
                [0]_.Seed.Nonterminal.Base        Item
      [2]_.Seed.Rule                              Label = string
        [0]_.Seed.Nonterminal                     Label
          [0]_.Seed.Nonterminal.Base              Label
        [1]_.Seed.ExprOrAlias                     = string
          [0]_.Seed.Terminal                      string
      [3]_.Seed.Rule                              Item = ... | number
        [0]_.Seed.Nonterminal                     Item
          [0]_.Seed.Nonterminal.Base              Item
        [1]_.Seed.ExprOrAlias                     = x ... | number
          [0]_.Seed.Expr.Or.|                     x | string | number
            [0]_.Seed.NonterminalMaybeVar         x
              [0]_.Seed.Nonterminal               x
                [0]_.Seed.Nonterminal.Base        x
            [1]_.Seed.Terminal                    string
            [2]_.Seed.Terminal                    number
)";

    const string_t pt_str_1 = SILVA_EXPECT_REQUIRE(sf_seed_pt_1->span().to_string());
    const string_t pt_str_2 = SILVA_EXPECT_REQUIRE(sf_seed_pt_2->span().to_string());
    CHECK(pt_str_1 == expected.substr(1));
    CHECK(pt_str_2 == expected.substr(1));

    seed_engine_t se(sw.ptr());
    SILVA_EXPECT_REQUIRE(se.add(sf_seed_pt_1->span()));
    REQUIRE(se.rule_exprs.size() == 4);
    using rfl::json::write;
    const name_id_t ni_sf       = sw.name_id_of("SimpleFern");
    const name_id_t ni_li       = sw.name_id_of(ni_sf, "LabeledItem");
    const name_id_t ni_l        = sw.name_id_of(ni_sf, "Label");
    const name_id_t ni_i        = sw.name_id_of(ni_sf, "Item");
    const parse_tree_span_t pts = sf_seed_pt_1->span();
    CHECK(se.rule_exprs.at(ni_sf) == pts.sub_tree_span_at(8));
    CHECK(se.rule_exprs.at(ni_li) == pts.sub_tree_span_at(23));
    CHECK(se.rule_exprs.at(ni_l) == pts.sub_tree_span_at(38));
    CHECK(se.rule_exprs.at(ni_i) == pts.sub_tree_span_at(43));
    REQUIRE(se.nonterminal_rules.size() == 4);
    CHECK(se.nonterminal_rules.at(pts.sub_tree_span_at(15)) == ni_li);
    CHECK(se.nonterminal_rules.at(pts.sub_tree_span_at(29)) == ni_l);
    CHECK(se.nonterminal_rules.at(pts.sub_tree_span_at(33)) == ni_i);
    CHECK(se.nonterminal_rules.at(pts.sub_tree_span_at(46)) == ni_sf);

    const string_t sf_code = R"'( [ 'abc' ; [ 'def' 123 ] 'jkl' ;])'";
    const auto sf_tt       = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "", sf_code));
    const auto sfpt        = SILVA_EXPECT_REQUIRE(se.apply(sf_tt, ni_sf));

    const std::string_view expected_parse_tree = R"(
[0]_.SimpleFern                                   [ 'abc' ... ; ]
  [0]_.SimpleFern.LabeledItem                     'abc'
    [0]_.SimpleFern.Item                          'abc'
  [1]_.SimpleFern.LabeledItem                     [ 'def' 123 ]
    [0]_.SimpleFern.Item                          [ 'def' 123 ]
      [0]_.SimpleFern                             [ 'def' 123 ]
        [0]_.SimpleFern.LabeledItem               'def'
          [0]_.SimpleFern.Item                    'def'
        [1]_.SimpleFern.LabeledItem               123
          [0]_.SimpleFern.Item                    123
  [2]_.SimpleFern.LabeledItem                     'jkl'
    [0]_.SimpleFern.Item                          'jkl'
)";
    const string_t result{SILVA_EXPECT_REQUIRE(sfpt->span().to_string())};
    CHECK(result == expected_parse_tree.substr(1));
  }
}
