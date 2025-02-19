#include "parse_axe.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using namespace silva::parse_axe;

using enum assoc_t;

TEST_CASE("parse-axe", "[parse_axe_t]")
{
  token_context_t tc;
  vector_t<parse_axe_level_desc_t> level_descs;
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "dot"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {infix_t{tc.token_id(".")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "sub"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {postfix_nest_t{tc.token_id("["), tc.token_id("]")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "dol"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {postfix_t{tc.token_id("$")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "exc"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {postfix_t{tc.token_id("!")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "til"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {prefix_t{tc.token_id("~")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "prf"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {prefix_t{tc.token_id("+")}, prefix_t{tc.token_id("-")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "mul"),
      .assoc = LEFT_TO_RIGHT,
      .opers = {infix_t{tc.token_id("*")}, infix_t{tc.token_id("/")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "add"),
      .assoc = FLAT,
      .opers = {infix_t{tc.token_id("+")}, infix_t{tc.token_id("-")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "ter"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {ternary_t{tc.token_id("?"), tc.token_id(":")}},
  });
  level_descs.push_back(parse_axe_level_desc_t{
      .name  = tc.full_name_id_of("test", "eqa"),
      .assoc = RIGHT_TO_LEFT,
      .opers = {infix_t{tc.token_id("=")}},
  });
  const auto pa = SILVA_EXPECT_REQUIRE(
      parse_axe_create(tc.ptr(), primary_nest_t{tc.token_id("("), tc.token_id(")")}, level_descs));
  CHECK(!pa.concat.has_value());
  CHECK(pa.results.size() == 15);
  CHECK(pa.results.at(tc.token_id("=")) ==
        result_t{.prefix = none,
                 .regular =
                     result_oper_t<oper_regular_t>{
                         .oper       = infix_t{tc.token_id("=")},
                         .level_name = tc.full_name_id_of("test", "eqa"),
                         .precedence = precedence_t{.level_index = 1, .assoc = RIGHT_TO_LEFT},
                     },
                 .is_right_bracket = false});
}
