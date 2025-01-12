#include "shunting_yard.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

namespace {
  constexpr array_t<char, 6> ops = {'~', '+', '-', '*', '/', '!'};
  constexpr token_id_t op_tilda  = 0;
  constexpr token_id_t op_add    = 1;
  constexpr token_id_t op_sub    = 2;
  constexpr token_id_t op_mul    = 3;
  constexpr token_id_t op_div    = 4;
  constexpr token_id_t op_fact   = 5;
}

TEST_CASE("shunting-yard", "[shunting_yard_t]")
{
  using level_index_t = shunting_yard_t::level_index_t;
  using enum shunting_yard_t::level_type_t;
  shunting_yard_t sy;
  const level_index_t lvl1 = sy.add_level(PREFIX);
  const level_index_t lvl2 = sy.add_level(BINARY_LEFT_TO_RIGHT);
  const level_index_t lvl3 = sy.add_level(BINARY_LEFT_TO_RIGHT);
  const level_index_t lvl4 = sy.add_level(POSTFIX);
  sy.add_operator(lvl1, op_tilda);
  sy.add_operator(lvl2, op_mul);
  sy.add_operator(lvl2, op_div);
  sy.add_operator(lvl3, op_add);
  sy.add_operator(lvl3, op_sub);
  sy.add_operator(lvl4, op_fact);
  CHECK(sy.has_operator(op_tilda));
  CHECK(sy.has_operator(op_div));
  CHECK(sy.has_operator(op_fact));

  shunting_yard_run_t run{
      .shunting_yard = &sy,
      .callback      = [&](const span_t<const Expression> exprs,
                      const token_id_t token_id,
                      const shunting_yard_t::level_index_t level_index) -> Expression {
        string_t retval;
        retval += '(';
        for (index_t i = 0; i < exprs.size(); ++i) {
          if (i > 0) {
            retval += ops.at(token_id);
          }
          retval += exprs[i].repr;
        }
        retval += ')';
        return {std::move(retval)};
      }};
  SILVA_EXPECT_REQUIRE(run.push_back(Expression{"a"}));
  SILVA_EXPECT_REQUIRE(run.push_back(op_add));
  SILVA_EXPECT_REQUIRE(run.push_back(Expression{"b"}));
  SILVA_EXPECT_REQUIRE(run.push_back(op_mul));
  SILVA_EXPECT_REQUIRE(run.push_back(Expression{"c"}));
  SILVA_EXPECT_REQUIRE(run.push_back(op_add));
  SILVA_EXPECT_REQUIRE(run.push_back(Expression{"d"}));
  const auto result = SILVA_EXPECT_REQUIRE(run.finish());
  REQUIRE(result.repr == "(a+(b*c)+d)");
}

// ~ ~ a ! ! + ~ ~ b ! !
// a + b !
// a + b ! + c
// a ++ b
