#include "parse_axe.hpp"
#include "tokenization.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using namespace silva::parse_axe;

using enum assoc_t;

namespace silva::test {
  template<typename ParseAxeNursery>
  expected_t<unique_ptr_t<parse_tree_t>>
  run_parse_axe(const parse_axe_t& parse_axe, shared_ptr_t<const tokenization_t> tokenization)
  {
    expected_traits_t expected_traits{.materialize_fwd = true};
    const index_t n = tokenization->tokens.size();
    ParseAxeNursery nursery(parse_axe, std::move(tokenization));
    const parse_tree_node_t sub = SILVA_EXPECT_FWD(nursery.expression());
    SILVA_EXPECT(sub.num_children == 1, ASSERT);
    SILVA_EXPECT(sub.subtree_size == nursery.tree.size(), ASSERT);
    SILVA_EXPECT(nursery.token_index == n, MAJOR, "Tokens left after parsing fern.");
    return {std::make_unique<parse_tree_t>(std::move(nursery).finish())};
  }

  template<typename ParseAxeNursery>
  void test_parse_axe(token_catalog_ptr_t tcp,
                      const parse_axe_t& pa,
                      const string_view_t text,
                      const optional_t<string_view_t> expected_str)
  {
    INFO(text);
    auto maybe_tt = tokenize(tcp, "", string_t{text});
    REQUIRE(maybe_tt.has_value());
    auto tt              = std::move(maybe_tt).value();
    auto maybe_result_pt = run_parse_axe<ParseAxeNursery>(pa, share(std::move(tt)));
    REQUIRE(maybe_result_pt.has_value() == expected_str.has_value());
    if (!expected_str.has_value()) {
      return;
    }
    auto result_pt        = std::move(maybe_result_pt).value();
    const auto result_str = SILVA_EXPECT_REQUIRE(result_pt->span().to_string());
    CHECK(result_str == expected_str.value().substr(1));
  }

  TEST_CASE("parse-axe-basic", "[parse_axe_t]")
  {
    struct test_nursery_t : public parse_tree_nursery_t {
      const parse_axe_t& parse_axe;

      test_nursery_t(const parse_axe_t& parse_axe, shared_ptr_t<const tokenization_t> tokenization)
        : parse_tree_nursery_t(tokenization), parse_axe(parse_axe)
      {
      }

      expected_t<parse_tree_node_t> atom()
      {
        auto ss_rule = stake();
        ss_rule.create_node(tcp->name_id_of("test", "atom"));
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
        return parse_axe.apply(*this, tcp->name_id_of("atom"), dg);
      }
    };

    token_catalog_t tc;
    vector_t<parse_axe_level_desc_t> level_descs;
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("nst"),
        .assoc     = NEST,
        .opers     = {atom_nest_t{*tc.token_id("("), *tc.token_id(")")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("dot"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {infix_t{*tc.token_id(".")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("sub"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {postfix_nest_t{*tc.token_id("["), *tc.token_id("]")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("dol"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {postfix_t{*tc.token_id("$")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("exc"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {postfix_t{*tc.token_id("!")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("til"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {prefix_t{*tc.token_id("~")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("prf"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {prefix_t{*tc.token_id("+")}, prefix_t{*tc.token_id("-")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("mul"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{*tc.token_id("*")}, infix_t{*tc.token_id("/")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("add"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{*tc.token_id("+")}, infix_t{*tc.token_id("-")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("ter"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {ternary_t{*tc.token_id("?"), *tc.token_id(":")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("eqa"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {infix_t{*tc.token_id("=")}},
    });
    const auto pa =
        SILVA_EXPECT_REQUIRE(parse_axe_create(tc.ptr(), tc.name_id_of("expr"), level_descs));
    CHECK(!pa.concat_result.has_value());
    CHECK(pa.results.size() == 15);
    CHECK(pa.results.at(*tc.token_id("=")) ==
          parse_axe_result_t{
              .prefix = none,
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = infix_t{*tc.token_id("=")},
                      .name       = tc.name_id_of("expr", "eqa", "="),
                      .precedence = precedence_t{.level_index = 1, .assoc = RIGHT_TO_LEFT},
                  },
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*tc.token_id("?")) ==
          parse_axe_result_t{
              .prefix = none,
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = ternary_t{*tc.token_id("?"), *tc.token_id(":")},
                      .name       = tc.name_id_of("expr", "ter", "?"),
                      .precedence = precedence_t{.level_index = 2, .assoc = RIGHT_TO_LEFT},
                  },
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*tc.token_id(":")) ==
          parse_axe_result_t{
              .prefix           = none,
              .regular          = none,
              .is_right_bracket = true,
          });
    CHECK(pa.results.at(*tc.token_id("+")) ==
          parse_axe_result_t{
              .prefix =
                  result_oper_t<oper_prefix_t>{
                      .oper       = prefix_t{*tc.token_id("+")},
                      .name       = tc.name_id_of("expr", "prf", "+"),
                      .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                  },
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = infix_t{*tc.token_id("+")},
                      .name       = tc.name_id_of("expr", "add", "+"),
                      .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                  },
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*tc.token_id("-")) ==
          parse_axe_result_t{
              .prefix =
                  result_oper_t<oper_prefix_t>{
                      .oper       = prefix_t{*tc.token_id("-")},
                      .name       = tc.name_id_of("expr", "prf", "-"),
                      .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                  },
              .regular =
                  result_oper_t<oper_regular_t>{
                      .oper       = infix_t{*tc.token_id("-")},
                      .name       = tc.name_id_of("expr", "add", "-"),
                      .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                  },
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*tc.token_id("(")) ==
          parse_axe_result_t{
              .prefix =
                  result_oper_t<oper_prefix_t>{
                      .oper       = atom_nest_t{*tc.token_id("("), *tc.token_id(")")},
                      .name       = tc.name_id_of("expr", "nst", "("),
                      .precedence = precedence_t{.level_index = 11, .assoc = NEST},
                  },
              .regular          = none,
              .is_right_bracket = false,
          });
    CHECK(pa.results.at(*tc.token_id(")")) ==
          parse_axe_result_t{
              .prefix           = none,
              .regular          = none,
              .is_right_bracket = true,
          });

    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1", R"(
[0]_.test.atom                                    1
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 + 2", R"(
[0]_.expr.add.+                                   1 + 2
  [0]_.test.atom                                  1
  [1]_.test.atom                                  2
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 - 2", R"(
[0]_.expr.add.-                                   1 - 2
  [0]_.test.atom                                  1
  [1]_.test.atom                                  2
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 + 2 * 3 + 4", R"(
[0]_.expr.add.+                                   1 + ... + 4
  [0]_.expr.add.+                                 1 + 2 * 3
    [0]_.test.atom                                1
    [1]_.expr.mul.*                               2 * 3
      [0]_.test.atom                              2
      [1]_.test.atom                              3
  [1]_.test.atom                                  4
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(),
                                         pa,
                                         "1 - 2 + f . g . h * 3 / 4",
                                         R"(
[0]_.expr.add.+                                   1 - ... / 4
  [0]_.expr.add.-                                 1 - 2
    [0]_.test.atom                                1
    [1]_.test.atom                                2
  [1]_.expr.mul./                                 f . ... / 4
    [0]_.expr.mul.*                               f . ... * 3
      [0]_.expr.dot..                             f . g . h
        [0]_.test.atom                            f
        [1]_.expr.dot..                           g . h
          [0]_.test.atom                          g
          [1]_.test.atom                          h
      [1]_.test.atom                              3
    [1]_.test.atom                                4
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 ! + 3", R"(
[0]_.expr.add.+                                   2 ! + 3
  [0]_.expr.exc.!                                 2 !
    [0]_.test.atom                                2
  [1]_.test.atom                                  3
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, " - + 1", R"(
[0]_.expr.prf.-                                   - + 1
  [0]_.expr.prf.+                                 + 1
    [0]_.test.atom                                1
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + - + 1", R"(
[0]_.expr.add.+                                   a + - + 1
  [0]_.test.atom                                  a
  [1]_.expr.prf.-                                 - + 1
    [0]_.expr.prf.+                               + 1
      [0]_.test.atom                              1
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "- - 1 * 2", R"(
[0]_.expr.mul.*                                   - - 1 * 2
  [0]_.expr.prf.-                                 - - 1
    [0]_.expr.prf.-                               - 1
      [0]_.test.atom                              1
  [1]_.test.atom                                  2
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "- - 1 . 2", R"(
[0]_.expr.prf.-                                   - - 1 . 2
  [0]_.expr.prf.-                                 - 1 . 2
    [0]_.expr.dot..                               1 . 2
      [0]_.test.atom                              1
      [1]_.test.atom                              2
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 . 2 !", R"(
[0]_.expr.exc.!                                   1 . 2 !
  [0]_.expr.dot..                                 1 . 2
    [0]_.test.atom                                1
    [1]_.test.atom                                2
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 + 2 !", R"(
[0]_.expr.add.+                                   1 + 2 !
  [0]_.test.atom                                  1
  [1]_.expr.exc.!                                 2 !
    [0]_.test.atom                                2
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 ! . 3", none);
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 . - 3", none);
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 $ !", R"(
[0]_.expr.exc.!                                   2 $ !
  [0]_.expr.dol.$                                 2 $
    [0]_.test.atom                                2
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 ! $", none);
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "+ ~ 2", R"(
[0]_.expr.prf.+                                   + ~ 2
  [0]_.expr.til.~                                 ~ 2
    [0]_.test.atom                                2
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "~ + 2", none);
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "( ( 0 ) )", R"(
[0]_.expr.nst.(                                   ( ( 0 ) )
  [0]_.expr.nst.(                                 ( 0 )
    [0]_.test.atom                                0
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 * ( 2 + 3 ) * 4", R"(
[0]_.expr.mul.*                                   1 * ... * 4
  [0]_.expr.mul.*                                 1 * ... 3 )
    [0]_.test.atom                                1
    [1]_.expr.nst.(                               ( 2 + 3 )
      [0]_.expr.add.+                             2 + 3
        [0]_.test.atom                            2
        [1]_.test.atom                            3
  [1]_.test.atom                                  4
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 * ( 2 + 3 ) * 4", R"(
[0]_.expr.mul.*                                   1 * ... * 4
  [0]_.expr.mul.*                                 1 * ... 3 )
    [0]_.test.atom                                1
    [1]_.expr.nst.(                               ( 2 + 3 )
      [0]_.expr.add.+                             2 + 3
        [0]_.test.atom                            2
        [1]_.test.atom                            3
  [1]_.test.atom                                  4
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a [ 0 ]", R"(
[0]_.expr.sub.[                                   a [ 0 ]
  [0]_.test.atom                                  a
  [1]_.test.atom                                  0
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a [ 0 ] [ 1 ]", R"(
[0]_.expr.sub.[                                   a [ ... 1 ]
  [0]_.expr.sub.[                                 a [ 0 ]
    [0]_.test.atom                                a
    [1]_.test.atom                                0
  [1]_.test.atom                                  1
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a [ 0 ] . b [ 1 ]", none);
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a [ 0 ] + b [ 1 ]", R"(
[0]_.expr.add.+                                   a [ ... 1 ]
  [0]_.expr.sub.[                                 a [ 0 ]
    [0]_.test.atom                                a
    [1]_.test.atom                                0
  [1]_.expr.sub.[                                 b [ 1 ]
    [0]_.test.atom                                b
    [1]_.test.atom                                1
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a ? b : c", R"(
[0]_.expr.ter.?                                   a ? b : c
  [0]_.test.atom                                  a
  [1]_.test.atom                                  b
  [2]_.test.atom                                  c
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a ? b : c ? d : e", R"(
[0]_.expr.ter.?                                   a ? ... : e
  [0]_.test.atom                                  a
  [1]_.test.atom                                  b
  [2]_.expr.ter.?                                 c ? d : e
    [0]_.test.atom                                c
    [1]_.test.atom                                d
    [2]_.test.atom                                e
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a ? b ? c : d : e", R"(
[0]_.expr.ter.?                                   a ? ... : e
  [0]_.test.atom                                  a
  [1]_.expr.ter.?                                 b ? c : d
    [0]_.test.atom                                b
    [1]_.test.atom                                c
    [2]_.test.atom                                d
  [2]_.test.atom                                  e
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a = b ? c = d : e = f", R"(
[0]_.expr.eqa.=                                   a = ... = f
  [0]_.test.atom                                  a
  [1]_.expr.eqa.=                                 b ? ... = f
    [0]_.expr.ter.?                               b ? ... : e
      [0]_.test.atom                              b
      [1]_.expr.eqa.=                             c = d
        [0]_.test.atom                            c
        [1]_.test.atom                            d
      [2]_.test.atom                              e
    [1]_.test.atom                                f
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b ? c + d : e + f", R"(
[0]_.expr.ter.?                                   a + ... + f
  [0]_.expr.add.+                                 a + b
    [0]_.test.atom                                a
    [1]_.test.atom                                b
  [1]_.expr.add.+                                 c + d
    [0]_.test.atom                                c
    [1]_.test.atom                                d
  [2]_.expr.add.+                                 e + f
    [0]_.test.atom                                e
    [1]_.test.atom                                f
)");
  }

  TEST_CASE("parse-axe-advanced", "[parse_axe_t]")
  {
    struct test_nursery_t : public parse_tree_nursery_t {
      const parse_axe_t& parse_axe;

      test_nursery_t(const parse_axe_t& parse_axe, shared_ptr_t<const tokenization_t> tokenization)
        : parse_tree_nursery_t(tokenization), parse_axe(parse_axe)
      {
      }

      expected_t<parse_tree_node_t> atom()
      {
        auto ss_rule = stake();
        ss_rule.create_node(tcp->name_id_of("test", "atom"));
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
        return parse_axe.apply(*this, tcp->name_id_of("atom"), dg);
      }
    };

    token_catalog_t tc;
    vector_t<parse_axe_level_desc_t> level_descs;
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("nst"),
        .assoc     = NEST,
        .opers     = {atom_nest_t{*tc.token_id("<<"), *tc.token_id(">>")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("prf_hi"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {prefix_nest_t{*tc.token_id("("), *tc.token_id(")")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("cat"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{.token_id = *tc.token_id("cc"), .concat = true}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("prf_lo"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {prefix_nest_t{*tc.token_id("{"), *tc.token_id("}")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("mul"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{*tc.token_id("*")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("add"),
        .assoc     = LEFT_TO_RIGHT,
        .opers     = {infix_t{.token_id = *tc.token_id("+"), .flatten = true},
                      infix_t{*tc.token_id("-")}},
    });
    level_descs.push_back(parse_axe_level_desc_t{
        .base_name = *tc.token_id("assign"),
        .assoc     = RIGHT_TO_LEFT,
        .opers     = {infix_t{.token_id = *tc.token_id("="), .flatten = true},
                      infix_t{*tc.token_id("%")}},
    });
    const auto pa =
        SILVA_EXPECT_REQUIRE(parse_axe_create(tc.ptr(), tc.name_id_of("expr"), level_descs));
    CHECK(pa.concat_result.has_value());
    CHECK(pa.results.size() == 11);

    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "x y z", R"(
[0]_.expr.cat.cc                                  x y z
  [0]_.expr.cat.cc                                x y
    [0]_.test.atom                                x
    [1]_.test.atom                                y
  [1]_.test.atom                                  z
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "{ b } a", R"(
[0]_.expr.prf_lo.{                                { b } a
  [0]_.test.atom                                  b
  [1]_.test.atom                                  a
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a { b } c", none);
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a ( b ) c", R"(
[0]_.expr.cat.cc                                  a ( b ) c
  [0]_.test.atom                                  a
  [1]_.expr.prf_hi.(                              ( b ) c
    [0]_.test.atom                                b
    [1]_.test.atom                                c
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a << { b } c >>", R"(
[0]_.expr.cat.cc                                  a << ... c >>
  [0]_.test.atom                                  a
  [1]_.expr.nst.<<                                << { ... c >>
    [0]_.expr.prf_lo.{                            { b } c
      [0]_.test.atom                              b
      [1]_.test.atom                              c
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "<< a { b } >> c", none);
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "x 1 x z", none);
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "x 1 { z", R"(
[0]_.expr.cat.cc                                  x 1 { z
  [0]_.expr.cat.cc                                x 1 {
    [0]_.test.atom                                x
    [1]_.test.atom                                1 {
  [1]_.test.atom                                  z
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b + c", R"(
[0]_.expr.add.+                                   a + b + c
  [0]_.test.atom                                  a
  [1]_.test.atom                                  b
  [2]_.test.atom                                  c
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b + c * d + e + f", R"(
[0]_.expr.add.+                                   a + ... + f
  [0]_.test.atom                                  a
  [1]_.test.atom                                  b
  [2]_.expr.mul.*                                 c * d
    [0]_.test.atom                                c
    [1]_.test.atom                                d
  [3]_.test.atom                                  e
  [4]_.test.atom                                  f
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b + c - d - e + f + g", R"(
[0]_.expr.add.+                                   a + ... + g
  [0]_.expr.add.-                                 a + ... - e
    [0]_.expr.add.-                               a + ... - d
      [0]_.expr.add.+                             a + b + c
        [0]_.test.atom                            a
        [1]_.test.atom                            b
        [2]_.test.atom                            c
      [1]_.test.atom                              d
    [1]_.test.atom                                e
  [1]_.test.atom                                  f
  [2]_.test.atom                                  g
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a - b + c + d - e", R"(
[0]_.expr.add.-                                   a - ... - e
  [0]_.expr.add.+                                 a - ... + d
    [0]_.expr.add.-                               a - b
      [0]_.test.atom                              a
      [1]_.test.atom                              b
    [1]_.test.atom                                c
    [2]_.test.atom                                d
  [1]_.test.atom                                  e
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b + c - d + e + f", R"(
[0]_.expr.add.+                                   a + ... + f
  [0]_.expr.add.-                                 a + ... - d
    [0]_.expr.add.+                               a + b + c
      [0]_.test.atom                              a
      [1]_.test.atom                              b
      [2]_.test.atom                              c
    [1]_.test.atom                                d
  [1]_.test.atom                                  e
  [2]_.test.atom                                  f
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a % b = c = d % e", R"(
[0]_.expr.assign.%                                a % ... % e
  [0]_.test.atom                                  a
  [1]_.expr.assign.=                              b = ... % e
    [0]_.test.atom                                b
    [1]_.test.atom                                c
    [2]_.expr.assign.%                            d % e
      [0]_.test.atom                              d
      [1]_.test.atom                              e
)");
    test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a = b = c % d = e = f", R"(
[0]_.expr.assign.=                                a = ... = f
  [0]_.test.atom                                  a
  [1]_.test.atom                                  b
  [2]_.expr.assign.%                              c % ... = f
    [0]_.test.atom                                c
    [1]_.expr.assign.=                            d = e = f
      [0]_.test.atom                              d
      [1]_.test.atom                              e
      [2]_.test.atom                              f
)");
  }
}
