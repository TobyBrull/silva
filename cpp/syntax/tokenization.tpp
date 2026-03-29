#include "syntax.hpp"
#include "syntax_farm.hpp"
#include "tokenization.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("tokenization", "[tokenization_t]")
  {
    syntax_farm_t sf;

    auto se = standard_seed_interpreter(sf.ptr());

    const string_view_t test_text = R"'(
      - Testor = tokenizer [
        - ignore WHITESPACE
        - ignore COMMENT
        - ignore INDENT
        - ignore DEDENT
        - ignore NEWLINE
        - name = IDENTIFIER
        - op = OPERATOR
      ]
)'";
    SILVA_REQUIRE(se->add_seed_text("testor.seed", string_t{test_text}));

    const string_view_t src = "x = a + b\ny = c * d\n";

    const auto fp = SILVA_REQUIRE(fragmentize(sf.ptr(), "test.seed", string_t{src}));
    const auto tp = SILVA_REQUIRE(se->tokenizer_farm.apply(fp, sf.name_id_of("Testor")));

    const string_view_t expected = R"'(
    TODO
)'";

    const auto result_str = pretty_string(*tp);
    CHECK(result_str == expected.substr(1));
  }
}
