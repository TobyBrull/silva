#include "canopy/string.hpp"
#include "unicode.hpp"

#include <catch2/catch_all.hpp>

namespace silva::unicode::test {
  TEST_CASE("unicode", "[codepoint_t]")
  {
    const auto null_f = [](const auto&...) -> expected_t<void> {
      return {};
    };

    SECTION("basic")
    {
      string_t s;
      utf8_encode_one(s, 0x0041);
      utf8_encode_one(s, 0x2299);
      utf8_encode_one(s, 0x00DF);
      utf8_encode_one(s, 0x20AC);
      utf8_encode_one(s, 0x10437);
      CHECK(s == "A⊙ß€𐐷");

      array_t<tuple_t<codepoint_t, index_t, index_t>> results;
      auto res = for_each_codepoint(
          string_view_t{s},
          [&](const codepoint_t a, const index_t b, const index_t c) -> expected_t<void> {
            results.emplace_back(a, b, c);
            return {};
          });
      SILVA_EXPECT_REQUIRE(std::move(res));

      const array_t<tuple_t<codepoint_t, index_t, index_t>> expected = {
          {0x0041, 0, 1},
          {0x2299, 1, 3},
          {0x00DF, 4, 2},
          {0x20AC, 6, 3},
          {0x10437, 9, 4},
      };
      CHECK(results == expected);
      CHECK(s.size() == 13);
    }
    SECTION("edge-cases")
    {
      string_t s;
      utf8_encode_one(s, 0x0000);
      utf8_encode_one(s, 0x007F);
      utf8_encode_one(s, 0x0080);
      utf8_encode_one(s, 0x07FF);
      utf8_encode_one(s, 0x0800);
      utf8_encode_one(s, 0xFFFF);
      utf8_encode_one(s, 0x10000);
      utf8_encode_one(s, 0x10FFFF);

      array_t<tuple_t<codepoint_t, index_t, index_t>> results;
      auto res = for_each_codepoint(
          string_view_t{s},
          [&](const codepoint_t a, const index_t b, const index_t c) -> expected_t<void> {
            results.emplace_back(a, b, c);
            return {};
          });
      SILVA_EXPECT_REQUIRE(std::move(res));

      const array_t<tuple_t<codepoint_t, index_t, index_t>> expected = {
          {0x0000, 0, 1},
          {0x007F, 1, 1},
          {0x0080, 2, 2},
          {0x07FF, 4, 2},
          {0x0800, 6, 3},
          {0xFFFF, 9, 3},
          {0x10000, 12, 4},
          {0x10FFFF, 16, 4},
      };
      CHECK(results == expected);
      CHECK(s.size() == 20);
    }
    SECTION("error_1")
    {
      string_t s;
      utf8_encode_one(s, 0x0041);
      utf8_encode_one(s, 0x110000);

      expected_t<void> res = for_each_codepoint(string_view_t{s}, null_f);
      REQUIRE(!res.has_value());
      const auto err_str = res.error().to_string_plain().as_string();
      CHECK_THAT(err_str, Catch::Matchers::ContainsSubstring("codepoint above 0x10FFFF"));
      CHECK_THAT(err_str, Catch::Matchers::ContainsSubstring("unable to decode codepoint at 1"));
    }
    SECTION("error_2")
    {
      string_t s;
      utf8_encode_one(s, 0x0041);
      utf8_encode_one(s, 0xDAFA);

      expected_t<void> res = for_each_codepoint(string_view_t{s}, null_f);
      REQUIRE(!res.has_value());
      const auto err_str = res.error().to_string_plain().as_string();
      CHECK_THAT(err_str, Catch::Matchers::ContainsSubstring("surrogate half"));
      CHECK_THAT(err_str, Catch::Matchers::ContainsSubstring("unable to decode codepoint at 1"));
    }
  }
}
