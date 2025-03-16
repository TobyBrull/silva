#include "tokenization.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("tokenization", "[tokenization_t]")
{
  token_context_t tc;
  using enum token_category_t;
  using info_t = token_info_t;
  {
    const auto result = SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "unit.test", "Hello   123 .<>."));
    REQUIRE(result->tokens.size() == 3);
    CHECK(*result->token_info_get(0) == info_t{"Hello", IDENTIFIER});
    CHECK(*result->token_info_get(1) == info_t{"123", NUMBER});
    CHECK(*result->token_info_get(2) == info_t{".<>.", OPERATOR});
    REQUIRE(tc.token_infos.size() == 4);
  }

  {
    const auto result = SILVA_EXPECT_REQUIRE(tokenize(tc.ptr(), "unit.test", R"(
        Silva 'Hel\'lo'  .(). # .().
        1 + 3
    )"));
    REQUIRE(result->tokens.size() == 9);
    CHECK(*result->token_info_get(0) == info_t{"Silva", IDENTIFIER});
    CHECK(*result->token_info_get(1) == info_t{"'Hel\\'lo'", STRING});
    CHECK(*result->token_info_get(2) == info_t{".", OPERATOR});
    CHECK(*result->token_info_get(3) == info_t{"(", OPERATOR});
    CHECK(*result->token_info_get(4) == info_t{")", OPERATOR});
    CHECK(*result->token_info_get(5) == info_t{".", OPERATOR});
    CHECK(*result->token_info_get(6) == info_t{"1", NUMBER});
    CHECK(*result->token_info_get(7) == info_t{"+", OPERATOR});
    CHECK(*result->token_info_get(8) == info_t{"3", NUMBER});
    REQUIRE(tc.token_infos.size() == 12);
  }

  {
    using vv_t = vector_t<name_id_t>;

    const name_id_t name1 = tc.name_id_of("std", "expr", "stmt");
    const name_id_t name2 = tc.name_id_of("std", "expr");
    const name_id_t name3 = tc.name_id_of("std", "ranges", "vector");
    CHECK(tc.name_infos.size() == 6);
    CHECK(tc.name_lookup.size() == 6);

    {
      const name_id_style_t ts{
          .tcp       = tc.ptr(),
          .root      = tc.token_id("cpp"),
          .current   = tc.token_id("this"),
          .parent    = tc.token_id("super"),
          .separator = tc.token_id("::"),
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
}
