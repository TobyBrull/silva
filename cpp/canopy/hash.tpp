#include "hash.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("hash")
{
  silva::hash('a');
  silva::hash(2);
  silva::hash(2ull);
  CHECK(silva::hash("abc") != 0);
  CHECK(silva::hash("abc") == silva::hash(silva::string_t{"abc"}));
  CHECK(silva::hash("abc") == silva::hash(silva::string_view_t{"abc"}));
  silva::hash(silva::pair_t<int, silva::string_t>{0, "abc"});
}

namespace silva::test {
  TEST_CASE("hash-silva")
  {
    hash('a');
    hash(2);
    hash(2ull);
    CHECK(hash("abc") != 0);
    CHECK(hash("abc") == hash(string_t{"abc"}));
    CHECK(hash("abc") == hash(string_view_t{"abc"}));
    hash(pair_t<int, string_t>{0, "abc"});
    hash(tuple_t<int, int, int>{0, 1, 2});
    hash(variant_t<int, int, int>{});
  }
}
