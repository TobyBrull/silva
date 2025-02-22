#include "parse_axe.hpp"
#include "tokenization.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using namespace silva::parse_axe;

using enum assoc_t;

namespace silva::test {
  struct parse_axe_parse_tree_nursery_t : public parse_tree_nursery_t {
    const parse_axe_t& parse_axe;

    parse_axe_parse_tree_nursery_t(const parse_axe_t& parse_axe,
                                   shared_ptr_t<const tokenization_t> tokenization)
      : parse_tree_nursery_t(tokenization), parse_axe(parse_axe)
    {
    }

    expected_t<parse_tree_sub_t> primary()
    {
      auto gg_rule = guard_for_rule();
      SILVA_EXPECT(num_tokens_left() >= 1, MINOR, "No token left for primary expression");
      SILVA_EXPECT(token_data_by()->category == token_category_t::NUMBER ||
                       token_data_by()->category == token_category_t::IDENTIFIER,
                   MINOR,
                   "Primary expression must be number of identifier");
      token_index += 1;
      return gg_rule.release();
    }

    expected_t<parse_tree_sub_t> expression()
    {
      auto dg = delegate_t<expected_t<parse_tree_sub_t>()>::make<
          &parse_axe_parse_tree_nursery_t::primary>(this);
      return parse_axe.apply(*this, tcp->full_name_id_of("primary"), dg);
    }
  };

  expected_t<unique_ptr_t<parse_tree_t>>
  run_parse_axe(const parse_axe_t& parse_axe, shared_ptr_t<const tokenization_t> tokenization)
  {
    expected_traits_t expected_traits{.materialize_fwd = true};
    const index_t n = tokenization->tokens.size();
    parse_axe_parse_tree_nursery_t nursery(parse_axe, std::move(tokenization));
    const parse_tree_sub_t sub = SILVA_EXPECT_FWD(nursery.expression());
    SILVA_EXPECT(sub.num_children == 1, ASSERT);
    SILVA_EXPECT(sub.num_children_total == nursery.retval.nodes.size(), ASSERT);
    SILVA_EXPECT(nursery.token_index == n, MAJOR, "Tokens left after parsing fern.");
    return {std::make_unique<parse_tree_t>(std::move(nursery.retval))};
  }

  void test_parse_axe(token_context_ptr_t tcp,
                      const parse_axe_t& pa,
                      const string_view_t text,
                      const string_view_t expected_str)
  {
    auto maybe_tt = tokenize(tcp, "", string_t{text});
    REQUIRE(maybe_tt.has_value());
    auto tt              = std::move(maybe_tt).value();
    auto maybe_result_pt = run_parse_axe(pa, share(std::move(tt)));
    REQUIRE(maybe_result_pt.has_value());
    auto result_pt        = std::move(maybe_result_pt).value();
    const auto result_str = parse_tree_to_string(*result_pt);
    CHECK(result_str == expected_str);
  }
}

TEST_CASE("parse-axe", "[parse_axe_t]")
{
  token_context_t tc;
  vector_t<parse_axe_level_desc_t> level_descs;
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
      .assoc = FLAT,
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
  const auto pa =
      SILVA_EXPECT_REQUIRE(parse_axe_create(tc.ptr(),
                                            tc.full_name_id_of("parseaxe"),
                                            primary_nest_t{tc.token_id("("), tc.token_id(")")},
                                            level_descs));
  CHECK(!pa.concat.has_value());
  CHECK(pa.results.size() == 15);
  CHECK(pa.results.at(tc.token_id("=")) ==
        result_t{
            .prefix = none,
            .regular =
                result_oper_t<oper_regular_t>{
                    .oper       = infix_t{tc.token_id("=")},
                    .level_name = tc.full_name_id_of("expr", "eqa"),
                    .precedence = precedence_t{.level_index = 1, .assoc = RIGHT_TO_LEFT},
                },
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id("?")) ==
        result_t{
            .prefix = none,
            .regular =
                result_oper_t<oper_regular_t>{
                    .oper       = ternary_t{tc.token_id("?"), tc.token_id(":")},
                    .level_name = tc.full_name_id_of("expr", "ter"),
                    .precedence = precedence_t{.level_index = 2, .assoc = RIGHT_TO_LEFT},
                },
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id(":")) ==
        result_t{
            .prefix           = none,
            .regular          = none,
            .is_right_bracket = true,
        });
  CHECK(pa.results.at(tc.token_id("+")) ==
        result_t{
            .prefix =
                result_oper_t<oper_prefix_t>{
                    .oper       = prefix_t{tc.token_id("+")},
                    .level_name = tc.full_name_id_of("expr", "prf"),
                    .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                },
            .regular =
                result_oper_t<oper_regular_t>{
                    .oper       = infix_t{tc.token_id("+")},
                    .level_name = tc.full_name_id_of("expr", "add"),
                    .precedence = precedence_t{.level_index = 3, .assoc = FLAT},
                },
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id("-")) ==
        result_t{
            .prefix =
                result_oper_t<oper_prefix_t>{
                    .oper       = prefix_t{tc.token_id("-")},
                    .level_name = tc.full_name_id_of("expr", "prf"),
                    .precedence = precedence_t{.level_index = 5, .assoc = RIGHT_TO_LEFT},
                },
            .regular =
                result_oper_t<oper_regular_t>{
                    .oper       = infix_t{tc.token_id("-")},
                    .level_name = tc.full_name_id_of("expr", "add"),
                    .precedence = precedence_t{.level_index = 3, .assoc = FLAT},
                },
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id("(")) ==
        result_t{
            .prefix =
                result_oper_t<oper_prefix_t>{
                    .oper       = primary_nest_t{tc.token_id("("), tc.token_id(")")},
                    .level_name = full_name_id_none,
                    .precedence =
                        precedence_t{.level_index = std::numeric_limits<level_index_t>::max(),
                                     .assoc       = INVALID},
                },
            .regular          = none,
            .is_right_bracket = false,
        });
  CHECK(pa.results.at(tc.token_id(")")) ==
        result_t{
            .prefix           = none,
            .regular          = none,
            .is_right_bracket = true,
        });

  //   test::test_parse_axe(tc.ptr(), pa, "1", R"(
  // [0].primary                               1
  // )");
  //   test::test_parse_axe(tc.ptr(), pa, "1+2", R"(
  // [0].expr.add                              1 + 2
  //   [0].primary                             1
  //   [1].primary                             2
  // )");
}
