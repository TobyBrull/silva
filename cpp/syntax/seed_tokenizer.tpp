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
    const string_view_t test_tok = R"'( Testy [
      - ignore NUMBER
      - include tokenizer FreeForm
      - name = [ '$' '@' ] IDENTIFIER
      - name = IDENTIFIER\'_t'
      - rel_path = IDENTIFIER ::: '/' '.' IDENTIFIER
      - op = ::: '=' '+'
    ] )'";

    const auto tt = SILVA_REQUIRE(tokenize(sw.ptr(), "test.tok", test_tok));
    const auto se = standard_seed_interpreter(sw.ptr());
    const auto pt = SILVA_REQUIRE(se->apply(tt, sw.name_id_of("Seed", "Tokenizer")));
  }
}
