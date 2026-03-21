#include "seed_tokenizer.hpp"

#include "syntax.hpp"
#include "syntax/parse_tree_nursery.hpp"

#include <catch2/catch_all.hpp>

using namespace silva;
using namespace silva::seed::impl;

namespace silva::seed::test {
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
      - language = LANGUAGE
    ] )'";

    const auto tt = SILVA_REQUIRE(tokenize(sw.ptr(), "test.tok", test_tok));
    const auto se = standard_seed_interpreter(sw.ptr());
    const auto pt = SILVA_REQUIRE(se->apply(tt, sw.name_id_of("Seed", "Tokenizer")));
    const auto tz = SILVA_REQUIRE(tokenizer_create(sw.ptr(), sw.name_id_of("Testor"), pt->span()));
    CHECK(tz.rules.size() == 10);

    const string_view_t src = "$hello ==+++ 42 array_t var/file.txt « a « c » « d » b » 1 @abc\n";
    const auto fr           = SILVA_REQUIRE(fragmentize("test.src", string_t{src}));
    const auto tp           = SILVA_REQUIRE(tz.apply(sw.ptr(), *fr));

    const auto ti_name = SILVA_REQUIRE(sw.token_id("name"));
    const auto ti_relp = SILVA_REQUIRE(sw.token_id("rel_path"));
    const auto ti_op   = SILVA_REQUIRE(sw.token_id("op"));
    const auto ti_lang = SILVA_REQUIRE(sw.token_id("language"));

    REQUIRE(tp->tokens.size() == 6);
    CHECK(sw.token_infos[tp->tokens[0]].str == "$hello");
    CHECK(sw.token_infos[tp->tokens[0]].category == ti_name);
    CHECK(sw.token_infos[tp->tokens[1]].str == "==+++");
    CHECK(sw.token_infos[tp->tokens[1]].category == ti_op);
    CHECK(sw.token_infos[tp->tokens[2]].str == "array_t");
    CHECK(sw.token_infos[tp->tokens[2]].category == ti_name);
    CHECK(sw.token_infos[tp->tokens[3]].str == "var/file.txt");
    CHECK(sw.token_infos[tp->tokens[3]].category == ti_relp);
    CHECK(sw.token_infos[tp->tokens[4]].category == ti_lang);
    CHECK(sw.token_infos[tp->tokens[5]].category == ti_name);
  }
}
