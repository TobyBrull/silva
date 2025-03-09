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
    const token_id_t ti_silva = tc.token_id("silva");
    const token_id_t ti_expr  = tc.token_id("expr");
    const token_id_t ti_func  = tc.token_id("func");
    const token_id_t ti_stmt  = tc.token_id("stmt");
    const token_id_t ti_comm  = tc.token_id("comm");

    using vv_t = vector_t<full_name_id_t>;

    const full_name_id_t name1 =
        tc.full_name_id_span(full_name_id_none, vv_t{ti_silva, ti_expr, ti_stmt});
    CHECK(tc.full_name_infos.size() == 4);
    CHECK(tc.full_name_lookup.size() == 4);
    CHECK(tc.full_name_to_string(name1) == "/silva/expr/stmt");
    CHECK(tc.full_name_to_string(name1, "::") == "::silva::expr::stmt");

    const full_name_id_t name2 =
        tc.full_name_id_span(full_name_id_none, vv_t{ti_silva, ti_expr, ti_expr});
    CHECK(tc.full_name_infos.size() == 5);
    CHECK(tc.full_name_lookup.size() == 5);
    CHECK(tc.full_name_to_string(name2, ".") == ".silva.expr.expr");

    const full_name_id_t name3 = tc.full_name_id_span(full_name_id_none, vv_t{ti_silva, ti_expr});
    CHECK(tc.full_name_infos.size() == 5);
    CHECK(tc.full_name_lookup.size() == 5);
    CHECK(tc.full_name_to_string(name3, "/") == "/silva/expr");

    const full_name_id_t fni_abcd = tc.full_name_id_of("A", "B", "C", "D");
    const full_name_id_t fni_abe  = tc.full_name_id_of("A", "B", "E");
    const full_name_id_t fni_ab   = tc.full_name_id_of("A", "B");
    CHECK(tc.full_name_id_lca(fni_abcd, fni_abe) == fni_ab);
    CHECK(tc.full_name_to_string_relative(fni_abcd, fni_abe) == "../../E");
    CHECK(tc.full_name_to_string_relative(fni_abe, fni_abcd) == "../C/D");
    CHECK(tc.full_name_to_string_relative(fni_ab, fni_abcd) == "C/D");
    CHECK(tc.full_name_to_string_relative(fni_abcd, fni_ab) == "../..");
  }
}
