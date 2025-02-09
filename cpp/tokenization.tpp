#include "tokenization.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;

TEST_CASE("tokenization", "[tokenization_t]")
{
  token_context_t tc;
  using enum token_category_t;
  using info_t = token_info_t;
  {
    const auto result = SILVA_EXPECT_REQUIRE(token_context_make("unit.test", "Hello   123 .<>."));
    REQUIRE(result.tokens.size() == 3);
    CHECK(*result.token_info_get(0) == info_t{"Hello", IDENTIFIER});
    CHECK(*result.token_info_get(1) == info_t{"123", NUMBER});
    CHECK(*result.token_info_get(2) == info_t{".<>.", OPERATOR});
    REQUIRE(token_context_t::get()->token_infos.size() == 3);
  }

  {
    const auto result = SILVA_EXPECT_REQUIRE(token_context_make("unit.test", R"(
        Silva "Hel\"lo"  .(). # .().
        1 + 3
    )"));
    REQUIRE(result.tokens.size() == 9);
    CHECK(*result.token_info_get(0) == info_t{"Silva", IDENTIFIER});
    CHECK(*result.token_info_get(1) == info_t{"\"Hel\\\"lo\"", STRING});
    CHECK(*result.token_info_get(2) == info_t{".", OPERATOR});
    CHECK(*result.token_info_get(3) == info_t{"(", OPERATOR});
    CHECK(*result.token_info_get(4) == info_t{")", OPERATOR});
    CHECK(*result.token_info_get(5) == info_t{".", OPERATOR});
    CHECK(*result.token_info_get(6) == info_t{"1", NUMBER});
    CHECK(*result.token_info_get(7) == info_t{"+", OPERATOR});
    CHECK(*result.token_info_get(8) == info_t{"3", NUMBER});
    REQUIRE(token_context_t::get()->token_infos.size() == 11);
  }
}
