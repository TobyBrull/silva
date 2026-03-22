#include "seed_tokenizer.hpp"

#include "syntax.hpp"
#include "syntax/parse_tree_nursery.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using namespace silva::seed::impl;

namespace silva::seed::test {
  TEST_CASE("case-mask")
  {
    using enum case_mask_t;
    const auto c_silva  = std::to_underlying(SILVA_CASE);
    const auto c_snake  = std::to_underlying(SNAKE_CASE);
    const auto c_camel  = std::to_underlying(CAMEL_CASE);
    const auto c_pascal = std::to_underlying(PASCAL_CASE);
    const auto c_macro  = std::to_underlying(MACRO_CASE);
    const auto c_upper  = std::to_underlying(UPPER_CASE);
    const auto c_lower  = std::to_underlying(LOWER_CASE);

    const auto run = [](const string_view_t sv) -> case_mask_t {
      const auto retval = SILVA_REQUIRE(compute_case_mask(sv));
      return retval;
    };

    CHECK(run("hello-world") == static_cast<case_mask_t>(c_silva));
    CHECK(run("hello_world") == static_cast<case_mask_t>(c_snake));
    CHECK(run("helloWorld") == static_cast<case_mask_t>(c_camel));
    CHECK(run("HelloWorld") == static_cast<case_mask_t>(c_pascal));
    CHECK(run("HELLO_WORLD") == static_cast<case_mask_t>(c_macro));
    CHECK(run("HELLO") == static_cast<case_mask_t>(c_macro | c_pascal | c_upper));
    CHECK(run("hello") == static_cast<case_mask_t>(c_silva | c_snake | c_camel | c_lower));
  }
  TEST_CASE("seed-tokenizer")
  {
    syntax_ward_t sw;
    const string_view_t test_tok = R"'( tokenizer [
      - ignore NUMBER
      - ignore NEWLINE
      - ignore WHITESPACE
    # - include tokenizer FreeForm
      - name = [ '$' '@' ] IDENTIFIER
      - name = IDENTIFIER\'_t'
      - rel_path = IDENTIFIER ::: '/' '.' IDENTIFIER
      - op = ::: '=' '+'
    ] )'";

    const auto tt = SILVA_REQUIRE(tokenize(sw.ptr(), "test.tok", test_tok));
    const auto se = standard_seed_interpreter(sw.ptr());
    const auto pt = SILVA_REQUIRE(se->apply(tt, sw.name_id_of("Seed", "Tokenizer")));
    const auto tz = SILVA_REQUIRE(tokenizer_create(sw.ptr(), sw.name_id_of("Testor"), pt->span()));
    CHECK(tz.rules.size() == 9);

    const string_view_t src = "$hello ==+++ 42 array_t var/file.txt « a « c » « d » b » 1 @abc\n";
    const auto fr           = SILVA_REQUIRE(fragmentize(sw.ptr(), "test.src", string_t{src}));
    const auto tp           = SILVA_REQUIRE(tz.apply(fr));

    const auto ti_name = SILVA_REQUIRE(sw.token_id("name"));
    const auto ti_relp = SILVA_REQUIRE(sw.token_id("rel_path"));
    const auto ti_op   = SILVA_REQUIRE(sw.token_id("op"));
    const auto ti_lang = SILVA_REQUIRE(sw.token_id("language"));

    REQUIRE(tp->tokens.size() == 6);
    CHECK(tp->tokens[0] == *sw.token_id("$hello"));
    CHECK(tp->categories[0] == ti_name);
    CHECK(tp->tokens[1] == *sw.token_id("==+++"));
    CHECK(tp->categories[1] == ti_op);
    CHECK(tp->tokens[2] == *sw.token_id("array_t"));
    CHECK(tp->categories[2] == ti_name);
    CHECK(tp->tokens[3] == *sw.token_id("var/file.txt"));
    CHECK(tp->categories[3] == ti_relp);
    CHECK(tp->categories[4] == ti_lang);
    CHECK(tp->categories[5] == ti_name);
  }
}
