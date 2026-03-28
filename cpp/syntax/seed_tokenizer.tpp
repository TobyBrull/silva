#include "seed_tokenizer.hpp"

#include "syntax.hpp"

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
    CHECK(run("αλφα-βητα") == static_cast<case_mask_t>(c_silva));

    CHECK(run("hello_world") == static_cast<case_mask_t>(c_snake));
    CHECK(run("αλφα_βητα") == static_cast<case_mask_t>(c_snake));

    CHECK(run("helloWorld") == static_cast<case_mask_t>(c_camel));
    CHECK(run("αλφαΒητα") == static_cast<case_mask_t>(c_camel));

    CHECK(run("HelloWorld") == static_cast<case_mask_t>(c_pascal));
    CHECK(run("ΑλφαΒητα") == static_cast<case_mask_t>(c_pascal));

    CHECK(run("HELLO_WORLD") == static_cast<case_mask_t>(c_macro));
    CHECK(run("ΑΛΦΑ_ΒΗΤΑ") == static_cast<case_mask_t>(c_macro));

    CHECK(run("HELLO") == static_cast<case_mask_t>(c_macro | c_pascal | c_upper));
    CHECK(run("ΑΛΦΑ") == static_cast<case_mask_t>(c_macro | c_pascal | c_upper));

    CHECK(run("hello") == static_cast<case_mask_t>(c_silva | c_snake | c_camel | c_lower));
    CHECK(run("αλφα") == static_cast<case_mask_t>(c_silva | c_snake | c_camel | c_lower));
  }
  TEST_CASE("seed-tokenizer")
  {
    syntax_farm_t sf;
    const auto se = standard_seed_interpreter(sf.ptr());

    const auto ti_lang     = *sf.token_id("language");
    const auto ti_string   = *sf.token_id("string");
    const auto ti_freeform = *sf.token_id("FreeForm");
    const auto ti_testor   = *sf.token_id("Testor");
    const auto ti_name     = *sf.token_id("name");
    const auto ti_relp     = *sf.token_id("rel_path");
    const auto ti_op       = *sf.token_id("op");

    tokenizer_farm_t tf(sf.ptr());
    const auto load_tokenizer = [&](const token_id_t name, const string_view_t tokenizer_code) {
      const auto tt0 = SILVA_REQUIRE(tokenize(sf.ptr(), "", tokenizer_code));
      const auto pt0 = SILVA_REQUIRE(se->apply(tt0, sf.name_id_of("Seed", "Tokenizer")));
      SILVA_REQUIRE(tf.add(name, pt0->span()));
    };

    const string_view_t test_tok = R"'( [
      - ignore NUMBER
      - include tokenizer FreeForm
      - name = [ '$' '@' ] IDENTIFIER
      - name = IDENTIFIER\'_t'
      - rel_path = IDENTIFIER ::: '/' '.' IDENTIFIER
      - op = ::: '=' '+'
    ] )'";
    load_tokenizer(ti_testor, test_tok);

    const string_view_t free_form_tok = R"'( [
      - ignore WHITESPACE
      - ignore COMMENT
      - ignore INDENT
      - ignore DEDENT
      - ignore NEWLINE
      - number = NUMBER
      - string = STRING
    ] )'";
    load_tokenizer(ti_freeform, free_form_tok);

    CHECK(tf.tokenizers.at(ti_freeform).rules.size() == 7);
    CHECK(tf.tokenizers.at(ti_testor).rules.size() == 8);

    const string_view_t src = "$hello ==+++ 42 array_t var/file.txt « a « c » « d » b » 1 @abc\n";
    const auto fr           = SILVA_REQUIRE(fragmentize(sf.ptr(), "test.src", string_t{src}));
    const auto tp           = SILVA_REQUIRE(tf.apply(fr, ti_testor));

    REQUIRE(tp->tokens.size() == 6);
    CHECK(tp->tokens[0] == *sf.token_id("$hello"));
    CHECK(tp->categories[0] == ti_name);
    CHECK(tp->tokens[1] == *sf.token_id("==+++"));
    CHECK(tp->categories[1] == ti_op);
    CHECK(tp->tokens[2] == *sf.token_id("array_t"));
    CHECK(tp->categories[2] == ti_name);
    CHECK(tp->tokens[3] == *sf.token_id("var/file.txt"));
    CHECK(tp->categories[3] == ti_relp);
    CHECK(tp->categories[4] == ti_lang);
    CHECK(tp->categories[5] == ti_name);
  }
  TEST_CASE("bootstrap-seed-tokenizer")
  {
    syntax_farm_t sf;
    tokenizer_farm_t tf = make_bootstrap_tokenizer_farm(sf.ptr());
    auto se             = standard_seed_interpreter(sf.ptr());
    CHECK(tf == se->tokenizer_farm);
  }
}
