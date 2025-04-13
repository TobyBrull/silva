#include "convert.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("string conversion")
  {
    CHECK(string_escaped("Hello world") == R"(Hello world)");
    CHECK(string_escaped("abc\ndef\rghi\"jkl\\mno") == R"(abc\ndef\rghi\"jkl\\mno)");
    CHECK(string_escaped("abc\ndef\rghi\"jkl\\mno\\n") == R"(abc\ndef\rghi\"jkl\\mno\\n)");
    CHECK(string_unescaped(R"(abc\ndef\rghi\"jkl\\mno\\n)") == "abc\ndef\rghi\"jkl\\mno\\n");
  }
}
