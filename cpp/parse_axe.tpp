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
    const parse_tree_sub_t sub = SILVA_EXPECT_FWD(nursery.expression());
    SILVA_EXPECT(sub.num_children == 1, ASSERT);
    SILVA_EXPECT(sub.num_children_total == nursery.retval.nodes.size(), ASSERT);
    SILVA_EXPECT(nursery.token_index == n, MAJOR, "Tokens left after parsing fern.");
    return {std::make_unique<parse_tree_t>(std::move(nursery.retval))};
  }

  template<typename ParseAxeNursery>
  void test_parse_axe(token_context_ptr_t tcp,
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
    const auto result_str = SILVA_EXPECT_REQUIRE(parse_tree_to_string(*result_pt));
    CHECK(result_str == expected_str.value().substr(1));
  }
}

TEST_CASE("parse-axe-basic", "[parse_axe_t]")
{
  struct test_nursery_t : public parse_tree_nursery_t {
    const parse_axe_t& parse_axe;

    test_nursery_t(const parse_axe_t& parse_axe, shared_ptr_t<const tokenization_t> tokenization)
      : parse_tree_nursery_t(tokenization), parse_axe(parse_axe)
    {
    }

    expected_t<parse_tree_sub_t> atom()
    {
      auto gg_rule = guard_for_rule();
      gg_rule.set_rule_name(tcp->full_name_id_of("test", "atom"));
      SILVA_EXPECT(num_tokens_left() >= 1, MINOR, "No token left for atom expression");
      SILVA_EXPECT(token_data_by()->category == token_category_t::NUMBER ||
                       token_data_by()->category == token_category_t::IDENTIFIER,
                   MINOR);
      token_index += 1;
      return gg_rule.release();
    }

    expected_t<parse_tree_sub_t> expression()
    {
      using dg_t = delegate_t<expected_t<parse_tree_sub_t>()>;
      auto dg    = dg_t::make<&test_nursery_t::atom>(this);
      return parse_axe.apply(*this, tcp->full_name_id_of("atom"), dg);
    }
  };

  token_context_t tc;
  vector_t<parse_axe_level_desc_t> level_descs;
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "nst"),
      .assoc = NEST,
      .opers = {atom_nest_t{tc.token_id("("), tc.token_id(")")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "dot"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {infix_t{tc.token_id(".")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "sub"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {postfix_nest_t{tc.token_id("["), tc.token_id("]")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "dol"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {postfix_t{tc.token_id("$")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "exc"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {postfix_t{tc.token_id("!")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "til"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {prefix_t{tc.token_id("~")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "prf"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {prefix_t{tc.token_id("+")}, prefix_t{tc.token_id("-")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "mul"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {infix_t{tc.token_id("*")}, infix_t{tc.token_id("/")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "add"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {infix_t{tc.token_id("+")}, infix_t{tc.token_id("-")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "ter"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {ternary_t{tc.token_id("?"), tc.token_id(":")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "eqa"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {infix_t{tc.token_id("=")}},
  });
  const auto pa = SILVA_EXPECT_REQUIRE(parse_axe_create(tc.ptr(), level_descs));
  CHECK(!pa.has_concat);
  CHECK(pa.results.size() == 15);
  CHECK(pa.results.at(tc.token_id("=")) ==
        parse_axe_result_t{
            .prefix = none,
            .regular =
                result_oper_t<oper_regular_t>{
                    .oper       = infix_t{tc.token_id("=")},
                    .name       = tc.full_name_id_of("expr", "eqa", "="),
                    .precedence = precedence_t{.level_index = 1, .assoc = RIGHT_TO_LEFT},
                },
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id("?")) ==
        parse_axe_result_t{
            .prefix = none,
            .regular =
                result_oper_t<oper_regular_t>{
                    .oper       = ternary_t{tc.token_id("?"), tc.token_id(":")},
                    .name       = tc.full_name_id_of("expr", "ter", "?"),
                    .precedence = precedence_t{.level_index = 2, .assoc = RIGHT_TO_LEFT},
                },
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id(":")) ==
        parse_axe_result_t{
            .prefix           = none,
            .regular          = none,
            .is_right_bracket = true,
        });
  CHECK(pa.results.at(tc.token_id("+")) ==
        parse_axe_result_t{
            .prefix =
                result_oper_t<oper_prefix_t>{
                    .oper       = prefix_t{tc.token_id("+")},
                    .name       = tc.full_name_id_of("expr", "prf", "+"),
                    .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                },
            .regular =
                result_oper_t<oper_regular_t>{
                    .oper       = infix_t{tc.token_id("+")},
                    .name       = tc.full_name_id_of("expr", "add", "+"),
                    .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                },
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id("-")) ==
        parse_axe_result_t{
            .prefix =
                result_oper_t<oper_prefix_t>{
                    .oper       = prefix_t{tc.token_id("-")},
                    .name       = tc.full_name_id_of("expr", "prf", "-"),
                    .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                },
            .regular =
                result_oper_t<oper_regular_t>{
                    .oper       = infix_t{tc.token_id("-")},
                    .name       = tc.full_name_id_of("expr", "add", "-"),
                    .precedence = precedence_t{.level_index = 3, .assoc = LEFT_TO_RIGHT},
                },
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id("(")) ==
        parse_axe_result_t{
            .prefix =
                result_oper_t<oper_prefix_t>{
                    .oper       = atom_nest_t{tc.token_id("("), tc.token_id(")")},
                    .name       = tc.full_name_id_of("expr", "nst", "("),
                    .precedence = precedence_t{.level_index = 11, .assoc = NEST},
                },
            .regular          = none,
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id(")")) ==
        parse_axe_result_t{
            .prefix           = none,
            .regular          = none,
            .is_right_bracket = true,
        });

  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1", R"(
[0].test.atom                                     1
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 + 2", R"(
[0].expr.add.+                                    1 + 2
  [0].test.atom                                   1
  [1].test.atom                                   2
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 - 2", R"(
[0].expr.add.-                                    1 - 2
  [0].test.atom                                   1
  [1].test.atom                                   2
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 + 2 * 3 + 4", R"(
[0].expr.add.+                                    1 + 2 ...
  [0].expr.add.+                                  1 + 2 ...
    [0].test.atom                                 1
    [1].expr.mul.*                                2 * 3
      [0].test.atom                               2
      [1].test.atom                               3
  [1].test.atom                                   4
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(),
                                       pa,
                                       "1 - 2 + f . g . h * 3 / 4",
                                       R"(
[0].expr.add.+                                    1 - 2 ...
  [0].expr.add.-                                  1 - 2
    [0].test.atom                                 1
    [1].test.atom                                 2
  [1].expr.mul./                                  f . g ...
    [0].expr.mul.*                                f . g ...
      [0].expr.dot..                              f . g ...
        [0].test.atom                             f
        [1].expr.dot..                            g . h
          [0].test.atom                           g
          [1].test.atom                           h
      [1].test.atom                               3
    [1].test.atom                                 4
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 ! + 3", R"(
[0].expr.add.+                                    2 ! + ...
  [0].expr.exc.!                                  2 !
    [0].test.atom                                 2
  [1].test.atom                                   3
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, " - + 1", R"(
[0].expr.prf.-                                    - + 1
  [0].expr.prf.+                                  + 1
    [0].test.atom                                 1
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + - + 1", R"(
[0].expr.add.+                                    a + - ...
  [0].test.atom                                   a
  [1].expr.prf.-                                  - + 1
    [0].expr.prf.+                                + 1
      [0].test.atom                               1
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "- - 1 * 2", R"(
[0].expr.mul.*                                    - - 1 ...
  [0].expr.prf.-                                  - - 1
    [0].expr.prf.-                                - 1
      [0].test.atom                               1
  [1].test.atom                                   2
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "- - 1 . 2", R"(
[0].expr.prf.-                                    - - 1 ...
  [0].expr.prf.-                                  - 1 . ...
    [0].expr.dot..                                1 . 2
      [0].test.atom                               1
      [1].test.atom                               2
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 . 2 !", R"(
[0].expr.exc.!                                    1 . 2 ...
  [0].expr.dot..                                  1 . 2
    [0].test.atom                                 1
    [1].test.atom                                 2
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 + 2 !", R"(
[0].expr.add.+                                    1 + 2 ...
  [0].test.atom                                   1
  [1].expr.exc.!                                  2 !
    [0].test.atom                                 2
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 ! . 3", none);
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 . - 3", none);
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 $ !", R"(
[0].expr.exc.!                                    2 $ !
  [0].expr.dol.$                                  2 $
    [0].test.atom                                 2
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "2 ! $", none);
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "+ ~ 2", R"(
[0].expr.prf.+                                    + ~ 2
  [0].expr.til.~                                  ~ 2
    [0].test.atom                                 2
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "~ + 2", none);
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "( ( 0 ) )", R"(
[0].expr.nst.(                                    ( ( 0 ...
  [0].expr.nst.(                                  ( 0 )
    [0].test.atom                                 0
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 * ( 2 + 3 ) * 4", R"(
[0].expr.mul.*                                    1 * ( ...
  [0].expr.mul.*                                  1 * ( ...
    [0].test.atom                                 1
    [1].expr.nst.(                                ( 2 + ...
      [0].expr.add.+                              2 + 3
        [0].test.atom                             2
        [1].test.atom                             3
  [1].test.atom                                   4
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "1 * ( 2 + 3 ) * 4", R"(
[0].expr.mul.*                                    1 * ( ...
  [0].expr.mul.*                                  1 * ( ...
    [0].test.atom                                 1
    [1].expr.nst.(                                ( 2 + ...
      [0].expr.add.+                              2 + 3
        [0].test.atom                             2
        [1].test.atom                             3
  [1].test.atom                                   4
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a [ 0 ]", R"(
[0].expr.sub.[                                    a [ 0 ...
  [0].test.atom                                   a
  [1].test.atom                                   0
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a [ 0 ] [ 1 ]", R"(
[0].expr.sub.[                                    a [ 0 ...
  [0].expr.sub.[                                  a [ 0 ...
    [0].test.atom                                 a
    [1].test.atom                                 0
  [1].test.atom                                   1
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a [ 0 ] . b [ 1 ]", none);
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a [ 0 ] + b [ 1 ]", R"(
[0].expr.add.+                                    a [ 0 ...
  [0].expr.sub.[                                  a [ 0 ...
    [0].test.atom                                 a
    [1].test.atom                                 0
  [1].expr.sub.[                                  b [ 1 ...
    [0].test.atom                                 b
    [1].test.atom                                 1
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a ? b : c", R"(
[0].expr.ter.?                                    a ? b ...
  [0].test.atom                                   a
  [1].test.atom                                   b
  [2].test.atom                                   c
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a ? b : c ? d : e", R"(
[0].expr.ter.?                                    a ? b ...
  [0].test.atom                                   a
  [1].test.atom                                   b
  [2].expr.ter.?                                  c ? d ...
    [0].test.atom                                 c
    [1].test.atom                                 d
    [2].test.atom                                 e
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a ? b ? c : d : e", R"(
[0].expr.ter.?                                    a ? b ...
  [0].test.atom                                   a
  [1].expr.ter.?                                  b ? c ...
    [0].test.atom                                 b
    [1].test.atom                                 c
    [2].test.atom                                 d
  [2].test.atom                                   e
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a = b ? c = d : e = f", R"(
[0].expr.eqa.=                                    a = b ...
  [0].test.atom                                   a
  [1].expr.eqa.=                                  b ? c ...
    [0].expr.ter.?                                b ? c ...
      [0].test.atom                               b
      [1].expr.eqa.=                              c = d
        [0].test.atom                             c
        [1].test.atom                             d
      [2].test.atom                               e
    [1].test.atom                                 f
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b ? c + d : e + f", R"(
[0].expr.ter.?                                    a + b ...
  [0].expr.add.+                                  a + b
    [0].test.atom                                 a
    [1].test.atom                                 b
  [1].expr.add.+                                  c + d
    [0].test.atom                                 c
    [1].test.atom                                 d
  [2].expr.add.+                                  e + f
    [0].test.atom                                 e
    [1].test.atom                                 f
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

    expected_t<parse_tree_sub_t> atom()
    {
      auto gg_rule = guard_for_rule();
      gg_rule.set_rule_name(tcp->full_name_id_of("test", "atom"));
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
      return gg_rule.release();
    }

    expected_t<parse_tree_sub_t> expression()
    {
      using dg_t = delegate_t<expected_t<parse_tree_sub_t>()>;
      auto dg    = dg_t::make<&test_nursery_t::atom>(this);
      return parse_axe.apply(*this, tcp->full_name_id_of("atom"), dg);
    }
  };

  token_context_t tc;
  vector_t<parse_axe_level_desc_t> level_descs;
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "nst"),
      .assoc = NEST,
      .opers = {atom_nest_t{tc.token_id("<<"), tc.token_id(">>")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "prf_hi"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {prefix_nest_t{tc.token_id("("), tc.token_id(")")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "cat"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {infix_t{token_id_none}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "prf_lo"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {prefix_nest_t{tc.token_id("{"), tc.token_id("}")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "mul"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {infix_t{tc.token_id("*")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "add"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {infix_t{.token_id = tc.token_id("+"), .flatten = true}, infix_t{tc.token_id("-")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("expr", "assign"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {infix_t{.token_id = tc.token_id("="), .flatten = true}, infix_t{tc.token_id("%")}},
  });
  const auto pa = SILVA_EXPECT_REQUIRE(parse_axe_create(tc.ptr(), level_descs));
  CHECK(pa.has_concat);
  CHECK(pa.results.size() == 12);

  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "x y z", R"(
[0].expr.cat.                                     x y z
  [0].expr.cat.                                   x y
    [0].test.atom                                 x
    [1].test.atom                                 y
  [1].test.atom                                   z
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "{ b } a", R"(
[0].expr.prf_lo.{                                 { b } ...
  [0].test.atom                                   b
  [1].test.atom                                   a
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a { b } c", none);
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a ( b ) c", R"(
[0].expr.cat.                                     a ( b ...
  [0].test.atom                                   a
  [1].expr.prf_hi.(                               ( b ) ...
    [0].test.atom                                 b
    [1].test.atom                                 c
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a << { b } c >>", R"(
[0].expr.cat.                                     a << { ...
  [0].test.atom                                   a
  [1].expr.nst.<<                                 << { b ...
    [0].expr.prf_lo.{                             { b } ...
      [0].test.atom                               b
      [1].test.atom                               c
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "<< a { b } >> c", none);
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "x 1 x z", none);
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "x 1 { z", R"(
[0].expr.cat.                                     x 1 { ...
  [0].expr.cat.                                   x 1 {
    [0].test.atom                                 x
    [1].test.atom                                 1 {
  [1].test.atom                                   z
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b + c", R"(
[0].expr.add.+                                    a + b ...
  [0].test.atom                                   a
  [1].test.atom                                   b
  [2].test.atom                                   c
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b + c * d + e + f", R"(
[0].expr.add.+                                    a + b ...
  [0].test.atom                                   a
  [1].test.atom                                   b
  [2].expr.mul.*                                  c * d
    [0].test.atom                                 c
    [1].test.atom                                 d
  [3].test.atom                                   e
  [4].test.atom                                   f
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b + c - d - e + f + g", R"(
[0].expr.add.+                                    a + b ...
  [0].expr.add.-                                  a + b ...
    [0].expr.add.-                                a + b ...
      [0].expr.add.+                              a + b ...
        [0].test.atom                             a
        [1].test.atom                             b
        [2].test.atom                             c
      [1].test.atom                               d
    [1].test.atom                                 e
  [1].test.atom                                   f
  [2].test.atom                                   g
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a - b + c + d - e", R"(
[0].expr.add.-                                    a - b ...
  [0].expr.add.+                                  a - b ...
    [0].expr.add.-                                a - b
      [0].test.atom                               a
      [1].test.atom                               b
    [1].test.atom                                 c
    [2].test.atom                                 d
  [1].test.atom                                   e
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a + b + c - d + e + f", R"(
[0].expr.add.+                                    a + b ...
  [0].expr.add.-                                  a + b ...
    [0].expr.add.+                                a + b ...
      [0].test.atom                               a
      [1].test.atom                               b
      [2].test.atom                               c
    [1].test.atom                                 d
  [1].test.atom                                   e
  [2].test.atom                                   f
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a % b = c = d % e", R"(
[0].expr.assign.%                                 a % b ...
  [0].test.atom                                   a
  [1].expr.assign.=                               b = c ...
    [0].test.atom                                 b
    [1].test.atom                                 c
    [2].expr.assign.%                             d % e
      [0].test.atom                               d
      [1].test.atom                               e
)");
  test::test_parse_axe<test_nursery_t>(tc.ptr(), pa, "a = b = c % d = e = f", R"(
[0].expr.assign.=                                 a = b ...
  [0].test.atom                                   a
  [1].test.atom                                   b
  [2].expr.assign.%                               c % d ...
    [0].test.atom                                 c
    [1].expr.assign.=                             d = e ...
      [0].test.atom                               d
      [1].test.atom                               e
      [2].test.atom                               f
)");
}
