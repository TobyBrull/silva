#include "seed_axe.hpp"

#include "syntax_ward.hpp"
#include "tokenization.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using namespace silva::impl;
using enum silva::impl::assoc_t;

namespace silva::test {
  template<typename ParseAxeNursery>
  expected_t<parse_tree_ptr_t>
  run_seed_axe(syntax_ward_t& sw, const seed_axe_t& seed_axe, tokenization_ptr_t tp)
  {
    const index_t n = tp->tokens.size();
    ParseAxeNursery nursery(seed_axe, std::move(tp));
    const parse_tree_node_t sub = SILVA_EXPECT_FWD(nursery.expression());
    SILVA_EXPECT(sub.num_children == 1, ASSERT);
    SILVA_EXPECT(sub.subtree_size == nursery.tree.size(), ASSERT);
    SILVA_EXPECT(nursery.token_index == n, MAJOR, "Tokens left after parsing fern.");
    return sw.add(std::move(nursery).finish());
  }

  template<typename ParseAxeNursery>
  void test_seed_axe(syntax_ward_ptr_t swp,
                     const seed_axe_t& pa,
                     const string_view_t text,
                     const optional_t<string_view_t> expected_str)
  {
    INFO(text);
    auto maybe_tt = tokenize(swp, "", string_t{text});
    REQUIRE(maybe_tt.has_value());
    auto tt              = std::move(maybe_tt).value();
    auto maybe_result_pt = run_seed_axe<ParseAxeNursery>(*swp, pa, std::move(tt));
    REQUIRE(maybe_result_pt.has_value() == expected_str.has_value());
    if (!expected_str.has_value()) {
      return;
    }
    auto result_pt        = std::move(maybe_result_pt).value();
    const auto result_str = SILVA_EXPECT_REQUIRE(result_pt->span().to_string());
    CHECK(result_str == expected_str.value().substr(1));
  }

  TEST_CASE("seed-axe-basic", "[seed_axe_t]")
  {
    struct test_nursery_t : public parse_tree_nursery_t {
      const seed_axe_t& seed_axe;

      test_nursery_t(const seed_axe_t& seed_axe, tokenization_ptr_t tp)
        : parse_tree_nursery_t(tp), seed_axe(seed_axe)
      {
      }

      expected_t<parse_tree_node_t> atom()
      {
        auto ss_rule = stake();
        ss_rule.create_node(swp->name_id_of("Test", "Atom"));
        SILVA_EXPECT(num_tokens_left() >= 1, MINOR, "No token left for atom expression");
        SILVA_EXPECT(token_data_by()->category == token_category_t::NUMBER ||
                         token_data_by()->category == token_category_t::IDENTIFIER,
                     MINOR);
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> expression()
      {
        using dg_t = delegate_t<expected_t<parse_tree_node_t>()>;
        auto dg    = dg_t::make<&test_nursery_t::atom>(this);
        return seed_axe.apply(*this, swp->name_id_of("Atom"), dg);
      }
    };

    syntax_ward_t sw;
    vector_t<seed_axe_level_desc_t> level_descs;
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Nst"),
        .assoc     = NEST,
        .opers     = {atom_nest_t{*sw.token_id("("), *sw.token_id(")")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Dot"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {infix_t{*sw.token_id(".")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Sub"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {postfix_nest_t{*sw.token_id("["), *sw.token_id("]")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Dol"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {postfix_t{*sw.token_id("$")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Exc"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {postfix_t{*sw.token_id("!")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Til"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {prefix_t{*sw.token_id("~")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Prf"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {prefix_t{*sw.token_id("+")}, prefix_t{*sw.token_id("-")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Mul"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{*sw.token_id("*")}, infix_t{*sw.token_id("/")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Add"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{*sw.token_id("+")}, infix_t{*sw.token_id("-")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Ter"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {ternary_t{*sw.token_id("?"), *sw.token_id(":")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Eqa"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {infix_t{*sw.token_id("=")}},
    });
    const auto pa =
        SILVA_EXPECT_REQUIRE(seed_axe_create(sw.ptr(), sw.name_id_of("Expr"), level_descs));
    CHECK(!pa.concat_result.has_value());
    CHECK(pa.results.size() == 15);
    CHECK(pa.results.at(*sw.token_id("=")) ==
          seed_axe_result_t{
              .prefix = none,
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = infix_t{*sw.token_id("=")},
                      .name       = sw.name_id_of("Expr", "Eqa", "="),
                      .precedence = precedence_t{.level_index = 1, .assoc = RIGHT_TO_LEFT},
                  },
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*sw.token_id("?")) ==
          seed_axe_result_t{
              .prefix = none,
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = ternary_t{*sw.token_id("?"), *sw.token_id(":")},
                      .name       = sw.name_id_of("Expr", "Ter", "?"),
                      .precedence = precedence_t{.level_index = 2, .assoc = RIGHT_TO_LEFT},
                  },
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*sw.token_id(":")) ==
          seed_axe_result_t{
              .prefix           = none,
              .regular          = none,
              .is_right_bracket = true,
          });
    CHECK(pa.results.at(*sw.token_id("+")) ==
          seed_axe_result_t{
              .prefix =
                  result_oper_t<oper_prefix_t>{
                      .oper       = prefix_t{*sw.token_id("+")},
                      .name       = sw.name_id_of("Expr", "Prf", "+"),
                      .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                  },
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = infix_t{*sw.token_id("+")},
                      .name       = sw.name_id_of("Expr", "Add", "+"),
                      .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                  },
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*sw.token_id("-")) ==
          seed_axe_result_t{
              .prefix =
                  result_oper_t<oper_prefix_t>{
                      .oper       = prefix_t{*sw.token_id("-")},
                      .name       = sw.name_id_of("Expr", "Prf", "-"),
                      .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                  },
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = infix_t{*sw.token_id("-")},
                      .name       = sw.name_id_of("Expr", "Add", "-"),
                      .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                  },
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*sw.token_id("(")) ==
          seed_axe_result_t{
              .prefix =
                  result_oper_t<oper_prefix_t>{
                      .oper       = atom_nest_t{*sw.token_id("("), *sw.token_id(")")},
                      .name       = sw.name_id_of("Expr", "Nst", "("),
                      .precedence = precedence_t{.level_index = 11, .assoc = NEST},
                  },
              .regular          = none,
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*sw.token_id(")")) ==
          seed_axe_result_t{
              .prefix           = none,
              .regular          = none,
              .is_right_bracket = true,
          });

    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "1", R"(
[0]_.Test.Atom                                    1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "1 + 2", R"(
[0]_.Expr.Add.+                                   1 + 2
  [0]_.Test.Atom                                  1
  [1]_.Test.Atom                                  2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "1 - 2", R"(
[0]_.Expr.Add.-                                   1 - 2
  [0]_.Test.Atom                                  1
  [1]_.Test.Atom                                  2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "1 + 2 * 3 + 4", R"(
[0]_.Expr.Add.+                                   1 + ... + 4
  [0]_.Expr.Add.+                                 1 + 2 * 3
    [0]_.Test.Atom                                1
    [1]_.Expr.Mul.*                               2 * 3
      [0]_.Test.Atom                              2
      [1]_.Test.Atom                              3
  [1]_.Test.Atom                                  4
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(),
                                        pa,
                                        "1 - 2 + f . g . h * 3 / 4",
                                        R"(
[0]_.Expr.Add.+                                   1 - ... / 4
  [0]_.Expr.Add.-                                 1 - 2
    [0]_.Test.Atom                                1
    [1]_.Test.Atom                                2
  [1]_.Expr.Mul./                                 f . ... / 4
    [0]_.Expr.Mul.*                               f . ... * 3
      [0]_.Expr.Dot..                             f . g . h
        [0]_.Test.Atom                            f
        [1]_.Expr.Dot..                           g . h
          [0]_.Test.Atom                          g
          [1]_.Test.Atom                          h
      [1]_.Test.Atom                              3
    [1]_.Test.Atom                                4
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "2 ! + 3", R"(
[0]_.Expr.Add.+                                   2 ! + 3
  [0]_.Expr.Exc.!                                 2 !
    [0]_.Test.Atom                                2
  [1]_.Test.Atom                                  3
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, " - + 1", R"(
[0]_.Expr.Prf.-                                   - + 1
  [0]_.Expr.Prf.+                                 + 1
    [0]_.Test.Atom                                1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a + - + 1", R"(
[0]_.Expr.Add.+                                   a + - + 1
  [0]_.Test.Atom                                  a
  [1]_.Expr.Prf.-                                 - + 1
    [0]_.Expr.Prf.+                               + 1
      [0]_.Test.Atom                              1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "- - 1 * 2", R"(
[0]_.Expr.Mul.*                                   - - 1 * 2
  [0]_.Expr.Prf.-                                 - - 1
    [0]_.Expr.Prf.-                               - 1
      [0]_.Test.Atom                              1
  [1]_.Test.Atom                                  2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "- - 1 . 2", R"(
[0]_.Expr.Prf.-                                   - - 1 . 2
  [0]_.Expr.Prf.-                                 - 1 . 2
    [0]_.Expr.Dot..                               1 . 2
      [0]_.Test.Atom                              1
      [1]_.Test.Atom                              2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "1 . 2 !", R"(
[0]_.Expr.Exc.!                                   1 . 2 !
  [0]_.Expr.Dot..                                 1 . 2
    [0]_.Test.Atom                                1
    [1]_.Test.Atom                                2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "1 + 2 !", R"(
[0]_.Expr.Add.+                                   1 + 2 !
  [0]_.Test.Atom                                  1
  [1]_.Expr.Exc.!                                 2 !
    [0]_.Test.Atom                                2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "2 ! . 3", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "2 . - 3", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "2 $ !", R"(
[0]_.Expr.Exc.!                                   2 $ !
  [0]_.Expr.Dol.$                                 2 $
    [0]_.Test.Atom                                2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "2 ! $", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "+ ~ 2", R"(
[0]_.Expr.Prf.+                                   + ~ 2
  [0]_.Expr.Til.~                                 ~ 2
    [0]_.Test.Atom                                2
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "~ + 2", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "( ( 0 ) )", R"(
[0]_.Expr.Nst.(                                   ( ( 0 ) )
  [0]_.Expr.Nst.(                                 ( 0 )
    [0]_.Test.Atom                                0
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "1 * ( 2 + 3 ) * 4", R"(
[0]_.Expr.Mul.*                                   1 * ... * 4
  [0]_.Expr.Mul.*                                 1 * ... 3 )
    [0]_.Test.Atom                                1
    [1]_.Expr.Nst.(                               ( 2 + 3 )
      [0]_.Expr.Add.+                             2 + 3
        [0]_.Test.Atom                            2
        [1]_.Test.Atom                            3
  [1]_.Test.Atom                                  4
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "1 * ( 2 + 3 ) * 4", R"(
[0]_.Expr.Mul.*                                   1 * ... * 4
  [0]_.Expr.Mul.*                                 1 * ... 3 )
    [0]_.Test.Atom                                1
    [1]_.Expr.Nst.(                               ( 2 + 3 )
      [0]_.Expr.Add.+                             2 + 3
        [0]_.Test.Atom                            2
        [1]_.Test.Atom                            3
  [1]_.Test.Atom                                  4
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a [ 0 ]", R"(
[0]_.Expr.Sub.[                                   a [ 0 ]
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  0
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a [ 0 ] [ 1 ]", R"(
[0]_.Expr.Sub.[                                   a [ ... 1 ]
  [0]_.Expr.Sub.[                                 a [ 0 ]
    [0]_.Test.Atom                                a
    [1]_.Test.Atom                                0
  [1]_.Test.Atom                                  1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a [ 0 ] . b [ 1 ]", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a [ 0 ] + b [ 1 ]", R"(
[0]_.Expr.Add.+                                   a [ ... 1 ]
  [0]_.Expr.Sub.[                                 a [ 0 ]
    [0]_.Test.Atom                                a
    [1]_.Test.Atom                                0
  [1]_.Expr.Sub.[                                 b [ 1 ]
    [0]_.Test.Atom                                b
    [1]_.Test.Atom                                1
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a ? b : c", R"(
[0]_.Expr.Ter.?                                   a ? b : c
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Test.Atom                                  c
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a ? b : c ? d : e", R"(
[0]_.Expr.Ter.?                                   a ? ... : e
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Expr.Ter.?                                 c ? d : e
    [0]_.Test.Atom                                c
    [1]_.Test.Atom                                d
    [2]_.Test.Atom                                e
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a ? b ? c : d : e", R"(
[0]_.Expr.Ter.?                                   a ? ... : e
  [0]_.Test.Atom                                  a
  [1]_.Expr.Ter.?                                 b ? c : d
    [0]_.Test.Atom                                b
    [1]_.Test.Atom                                c
    [2]_.Test.Atom                                d
  [2]_.Test.Atom                                  e
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a = b ? c = d : e = f", R"(
[0]_.Expr.Eqa.=                                   a = ... = f
  [0]_.Test.Atom                                  a
  [1]_.Expr.Eqa.=                                 b ? ... = f
    [0]_.Expr.Ter.?                               b ? ... : e
      [0]_.Test.Atom                              b
      [1]_.Expr.Eqa.=                             c = d
        [0]_.Test.Atom                            c
        [1]_.Test.Atom                            d
      [2]_.Test.Atom                              e
    [1]_.Test.Atom                                f
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a + b ? c + d : e + f", R"(
[0]_.Expr.Ter.?                                   a + ... + f
  [0]_.Expr.Add.+                                 a + b
    [0]_.Test.Atom                                a
    [1]_.Test.Atom                                b
  [1]_.Expr.Add.+                                 c + d
    [0]_.Test.Atom                                c
    [1]_.Test.Atom                                d
  [2]_.Expr.Add.+                                 e + f
    [0]_.Test.Atom                                e
    [1]_.Test.Atom                                f
)");
  }

  TEST_CASE("seed-axe-advanced", "[seed_axe_t]")
  {
    struct test_nursery_t : public parse_tree_nursery_t {
      const seed_axe_t& seed_axe;

      test_nursery_t(const seed_axe_t& seed_axe, tokenization_ptr_t tp)
        : parse_tree_nursery_t(tp), seed_axe(seed_axe)
      {
      }

      expected_t<parse_tree_node_t> atom()
      {
        auto ss_rule = stake();
        ss_rule.create_node(swp->name_id_of("Test", "Atom"));
        SILVA_EXPECT(num_tokens_left() >= 1, MINOR, "No token left for atom expression");
        if (token_data_by()->category == token_category_t::NUMBER) {
          SILVA_EXPECT(num_tokens_left() >= 2 &&
                           token_data_by(1)->category == token_category_t::OPERATOR,
                       MINOR);
          token_index += 2;
        }
        else {
          SILVA_EXPECT(token_data_by()->category == token_category_t::IDENTIFIER, MINOR);
          token_index += 1;
        }
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> expression()
      {
        using dg_t = delegate_t<expected_t<parse_tree_node_t>()>;
        auto dg    = dg_t::make<&test_nursery_t::atom>(this);
        return seed_axe.apply(*this, swp->name_id_of("Atom"), dg);
      }
    };

    syntax_ward_t sw;
    vector_t<seed_axe_level_desc_t> level_descs;
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Nst"),
        .assoc     = NEST,
        .opers     = {atom_nest_t{*sw.token_id("<<"), *sw.token_id(">>")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Prf_hi"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {prefix_nest_t{*sw.token_id("("), *sw.token_id(")")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Cat"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{.token_id = *sw.token_id("cc"), .concat = true}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Prf_lo"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {prefix_nest_t{*sw.token_id("{"), *sw.token_id("}")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Mul"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{*sw.token_id("*")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Add"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{.token_id = *sw.token_id("+"), .flatten = true},
                      infix_t{*sw.token_id("-")}},
    });
    level_descs.push_back(seed_axe_level_desc_t{
        .base_name = *sw.token_id("Assign"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {infix_t{.token_id = *sw.token_id("="), .flatten = true},
                      infix_t{*sw.token_id("%")}},
    });
    const auto pa =
        SILVA_EXPECT_REQUIRE(seed_axe_create(sw.ptr(), sw.name_id_of("Expr"), level_descs));
    CHECK(pa.concat_result.has_value());
    CHECK(pa.results.size() == 11);

    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "x y z", R"(
[0]_.Expr.Cat.cc                                  x y z
  [0]_.Expr.Cat.cc                                x y
    [0]_.Test.Atom                                x
    [1]_.Test.Atom                                y
  [1]_.Test.Atom                                  z
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "{ b } a", R"(
[0]_.Expr.Prf_lo.{                                { b } a
  [0]_.Test.Atom                                  b
  [1]_.Test.Atom                                  a
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a { b } c", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a ( b ) c", R"(
[0]_.Expr.Cat.cc                                  a ( b ) c
  [0]_.Test.Atom                                  a
  [1]_.Expr.Prf_hi.(                              ( b ) c
    [0]_.Test.Atom                                b
    [1]_.Test.Atom                                c
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a << { b } c >>", R"(
[0]_.Expr.Cat.cc                                  a << ... c >>
  [0]_.Test.Atom                                  a
  [1]_.Expr.Nst.<<                                << { ... c >>
    [0]_.Expr.Prf_lo.{                            { b } c
      [0]_.Test.Atom                              b
      [1]_.Test.Atom                              c
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "<< a { b } >> c", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "x 1 x z", none);
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "x 1 { z", R"(
[0]_.Expr.Cat.cc                                  x 1 { z
  [0]_.Expr.Cat.cc                                x 1 {
    [0]_.Test.Atom                                x
    [1]_.Test.Atom                                1 {
  [1]_.Test.Atom                                  z
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a + b + c", R"(
[0]_.Expr.Add.+                                   a + b + c
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Test.Atom                                  c
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a + b + c * d + e + f", R"(
[0]_.Expr.Add.+                                   a + ... + f
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Expr.Mul.*                                 c * d
    [0]_.Test.Atom                                c
    [1]_.Test.Atom                                d
  [3]_.Test.Atom                                  e
  [4]_.Test.Atom                                  f
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a + b + c - d - e + f + g", R"(
[0]_.Expr.Add.+                                   a + ... + g
  [0]_.Expr.Add.-                                 a + ... - e
    [0]_.Expr.Add.-                               a + ... - d
      [0]_.Expr.Add.+                             a + b + c
        [0]_.Test.Atom                            a
        [1]_.Test.Atom                            b
        [2]_.Test.Atom                            c
      [1]_.Test.Atom                              d
    [1]_.Test.Atom                                e
  [1]_.Test.Atom                                  f
  [2]_.Test.Atom                                  g
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a - b + c + d - e", R"(
[0]_.Expr.Add.-                                   a - ... - e
  [0]_.Expr.Add.+                                 a - ... + d
    [0]_.Expr.Add.-                               a - b
      [0]_.Test.Atom                              a
      [1]_.Test.Atom                              b
    [1]_.Test.Atom                                c
    [2]_.Test.Atom                                d
  [1]_.Test.Atom                                  e
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a + b + c - d + e + f", R"(
[0]_.Expr.Add.+                                   a + ... + f
  [0]_.Expr.Add.-                                 a + ... - d
    [0]_.Expr.Add.+                               a + b + c
      [0]_.Test.Atom                              a
      [1]_.Test.Atom                              b
      [2]_.Test.Atom                              c
    [1]_.Test.Atom                                d
  [1]_.Test.Atom                                  e
  [2]_.Test.Atom                                  f
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a % b = c = d % e", R"(
[0]_.Expr.Assign.%                                a % ... % e
  [0]_.Test.Atom                                  a
  [1]_.Expr.Assign.=                              b = ... % e
    [0]_.Test.Atom                                b
    [1]_.Test.Atom                                c
    [2]_.Expr.Assign.%                            d % e
      [0]_.Test.Atom                              d
      [1]_.Test.Atom                              e
)");
    test::test_seed_axe<test_nursery_t>(sw.ptr(), pa, "a = b = c % d = e = f", R"(
[0]_.Expr.Assign.=                                a = ... = f
  [0]_.Test.Atom                                  a
  [1]_.Test.Atom                                  b
  [2]_.Expr.Assign.%                              c % ... = f
    [0]_.Test.Atom                                c
    [1]_.Expr.Assign.=                            d = e = f
      [0]_.Test.Atom                              d
      [1]_.Test.Atom                              e
      [2]_.Test.Atom                              f
)");
  }
}
