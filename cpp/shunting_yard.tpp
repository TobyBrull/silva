#include "shunting_yard.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

namespace {
  constexpr array_t<char, 7> ops = {'~', '!', '+', '-', '*', '/', '?'};
  constexpr token_id_t op_tilda  = 0;
  constexpr token_id_t op_fact   = 1;
  constexpr token_id_t op_add    = 2;
  constexpr token_id_t op_sub    = 3;
  constexpr token_id_t op_mul    = 4;
  constexpr token_id_t op_div    = 5;
  constexpr token_id_t op_quest  = 6;
}

Expression
operator"" _E(const char* str, std::size_t)
{
  return Expression{str};
}

void
shunting_yard_test(shunting_yard_t* sy,
                   vector_t<variant_t<Expression, token_id_t>> items,
                   const string_view_t expected)
{
  INFO(expected);
  shunting_yard_run_t run{
      .shunting_yard = sy,
      .callback      = [&](const span_t<const Expression> exprs,
                      const token_id_t token_id,
                      const shunting_yard_t::level_index_t level_index) -> Expression {
        const shunting_yard_t::level_type_t type = sy->levels[level_index].type;
        using enum shunting_yard_t::level_type_t;
        string_t retval;
        retval += '(';
        if (type == PREFIX) {
          REQUIRE(exprs.size() == 1);
          retval += ops.at(token_id);
          retval += exprs.front().repr;
        }
        else if (type == POSTFIX) {
          REQUIRE(exprs.size() == 1);
          retval += exprs.front().repr;
          retval += ops.at(token_id);
        }
        else {
          for (index_t i = 0; i < exprs.size(); ++i) {
            if (i > 0) {
              retval += ops.at(token_id);
            }
            retval += exprs[i].repr;
          }
        }
        retval += ')';
        return {std::move(retval)};
      }};
  for (const auto& item: items) {
    std::visit([&](const auto& x) { SILVA_EXPECT_REQUIRE(run.push_back(x)); }, item);
  }
  const auto result = SILVA_EXPECT_REQUIRE(run.finish());
  CHECK(result.repr == expected);
}

TEST_CASE("shunting-yard", "[shunting_yard_t]")
{
  using level_index_t = shunting_yard_t::level_index_t;
  using enum shunting_yard_t::level_type_t;
  shunting_yard_t sy;
  const level_index_t lvl1 = sy.add_level(PREFIX);
  const level_index_t lvl2 = sy.add_level(POSTFIX);
  const level_index_t lvl3 = sy.add_level(BINARY_LEFT_TO_RIGHT);
  const level_index_t lvl4 = sy.add_level(BINARY_LEFT_TO_RIGHT);
  const level_index_t lvl5 = sy.add_level(POSTFIX);
  sy.add_operator(lvl1, op_tilda);
  sy.add_operator(lvl2, op_fact);
  sy.add_operator(lvl3, op_mul);
  sy.add_operator(lvl3, op_div);
  sy.add_operator(lvl4, op_add);
  sy.add_operator(lvl4, op_sub);
  sy.add_operator(lvl5, op_quest);
  CHECK(sy.has_operator(op_tilda));
  CHECK(sy.has_operator(op_div));
  CHECK(sy.has_operator(op_fact));

  shunting_yard_test(&sy, {"a"_E, op_add, "b"_E, op_mul, "c"_E, op_add, "d"_E}, "(a+(b*c)+d)");
  shunting_yard_test(&sy, {op_tilda, op_tilda, "a"_E, op_fact, op_fact}, "(((~(~a))!)!)");
  shunting_yard_test(&sy,
                     {op_tilda, "a"_E, op_add, "b"_E, op_add, "c"_E, op_fact},
                     "((~a)+b+(c!))");
  shunting_yard_test(&sy, {"a"_E, op_add, "b"_E, op_quest}, "((a+b)?)");
}
