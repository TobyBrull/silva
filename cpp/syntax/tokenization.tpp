#include "tokenization.hpp"

#include "syntax_ward.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("tokenization", "[tokenization_t]")
  {
    syntax_ward_t sw;
    using enum token_category_t;
    using info_t = token_info_t;
    {
      const auto result = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "unit.test", "Hello   123 .<>."));
      REQUIRE(result->tokens.size() == 3);
      CHECK(*result->token_info_get(0) == info_t{IDENTIFIER, "Hello"});
      CHECK(*result->token_info_get(1) == info_t{NUMBER, "123"});
      CHECK(*result->token_info_get(2) == info_t{OPERATOR, ".<>."});
      REQUIRE(sw.token_infos.size() == 8);
    }

    {
      const auto result = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "unit.test", R"(
        Silva 'Hel\'lo'  .(). # .().
        1 + 3
    )"));
      REQUIRE(result->tokens.size() == 9);
      CHECK(*result->token_info_get(0) == info_t{IDENTIFIER, "Silva"});
      CHECK(*result->token_info_get(1) == info_t{STRING, "'Hel\\'lo'"});
      CHECK(*result->token_info_get(2) == info_t{OPERATOR, "."});
      CHECK(*result->token_info_get(3) == info_t{OPERATOR, "("});
      CHECK(*result->token_info_get(4) == info_t{OPERATOR, ")"});
      CHECK(*result->token_info_get(5) == info_t{OPERATOR, "."});
      CHECK(*result->token_info_get(6) == info_t{NUMBER, "1"});
      CHECK(*result->token_info_get(7) == info_t{OPERATOR, "+"});
      CHECK(*result->token_info_get(8) == info_t{NUMBER, "3"});
      REQUIRE(sw.token_infos.size() == 15);
    }

    {
      using vv_t = vector_t<name_id_t>;

      const name_id_t name1 = sw.name_id_of("std", "expr", "stmt");
      const name_id_t name2 = sw.name_id_of("std", "expr");
      const name_id_t name3 = sw.name_id_of("std", "ranges", "vector");
      CHECK(sw.name_infos.size() == 6);
      CHECK(sw.name_lookup.size() == 6);

      {
        const name_id_style_t ts{
            .twp       = sw.ptr(),
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
    }

    {
      const string_t source_code   = R"([
  none
  true
  test: 'Hello'
  42.000000
  []


  [
    1.000000
    'two' : 2.000000
    3.000000
  ]
]
)";
      const string_view_t expected = R"(
[  0]   1:1   [
[  1]   2:3   none
[  2]   3:3   true
[  3]   4:3   test
[  4]   4:7   :
[  5]   4:9   'Hello'
[  6]   5:3   42.000000
[  7]   6:3   [
[  8]   6:4   ]
[  9]   9:3   [
[ 10]  10:5   1.000000
[ 11]  11:5   'two'
[ 12]  11:11  :
[ 13]  11:13  2.000000
[ 14]  12:5   3.000000
[ 15]  13:3   ]
[ 16]  14:1   ]
)";
      const auto tokenization = SILVA_EXPECT_REQUIRE(tokenize(sw.ptr(), "test.fern", source_code));
      const auto result_str   = to_string(*tokenization);
      CHECK(result_str.as_string_view() == expected.substr(1));
    }
  }
}
