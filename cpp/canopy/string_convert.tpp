#include "string_convert.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("string-conversion")
  {
    CHECK(string_escaped("Hello world") == R"(Hello world)");
    CHECK(string_escaped("abc\ndef\rghi\"jkl\\mno") == R"(abc\ndef\rghi\"jkl\\mno)");
    CHECK(string_escaped("abc\ndef\rghi\"jkl\\mno\\n") == R"(abc\ndef\rghi\"jkl\\mno\\n)");
    CHECK(string_unescaped(R"(abc\ndef\rghi\"jkl\\mno\\n)") == "abc\ndef\rghi\"jkl\\mno\\n");
  }

  TEST_CASE("enum-conversion")
  {
    enum class foo_t {
      ABC,
      XYZ,
    };
    const hash_map_t<string_t, foo_t>& hm = enum_hashmap_from_string<foo_t>();
    CHECK(convert_to<foo_t>("ABC") == foo_t::ABC);
    CHECK(convert_to<foo_t>("XYZ") == foo_t::XYZ);
  }
}
