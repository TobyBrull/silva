#include "unicode.hpp"

#include <catch2/catch_all.hpp>

namespace silva::unicode::test {
  TEST_CASE("unicode", "[codepoint_t]")
  {
    string_t s;
    utf8_encode_one(s, 0x0041);
    utf8_encode_one(s, 0x2299);
    utf8_encode_one(s, 0x00DF);
    utf8_encode_one(s, 0x20AC);
    utf8_encode_one(s, 0x10437);
    CHECK(s.size() == 13);
    CHECK(s == "A⊙ß€𐐷");

    array_t<tuple_t<codepoint_t, index_t, index_t>> results = {
        {0x0041, 0, 1},
        {0x2299, 1, 3},
        {0x00DF, 4, 2},
        {0x20AC, 6, 3},
        {0x10437, 9, 4},
    };
    auto res = for_each_codepoint(string_view_t{s},
                                  [&](const codepoint_t a, const index_t b, const index_t c) {
                                    results.emplace_back(a, b, c);
                                  });
    SILVA_EXPECT_REQUIRE(std::move(res));
  }
}
