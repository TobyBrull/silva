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
    const auto tp = SILVA_REQUIRE(se->tokenizer_farm.apply(fp, sf.token_id("Testor")));

    const string_view_t expected = R"'(
[  0]   1:1   cat=name                 x
[  1]   1:3   cat=op                   =
[  2]   1:5   cat=name                 a
[  3]   1:7   cat=op                   +
[  4]   1:9   cat=name                 b
[  5]   2:1   cat=name                 y
[  6]   2:3   cat=op                   =
[  7]   2:5   cat=name                 c
[  8]   2:7   cat=op                   *
[  9]   2:9   cat=name                 d
)'";

    const auto result_str = pretty_string(*tp);
    CHECK(result_str == expected.substr(1));
  }
}
