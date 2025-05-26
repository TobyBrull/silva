#include "cactus.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("cactus", "[cactus_t]")
  {
    cactus_t<string_t, int> cc;
    {
      const auto cp1 = SILVA_EXPECT_REQUIRE(cc.root().define("hello", 42));
      CHECK(cp1.get("world").has_value() == false);
      const auto cp1a = SILVA_EXPECT_REQUIRE(cp1.define("world", 100));
      const auto cp1b = SILVA_EXPECT_REQUIRE(cp1.define("world", 20));
      CHECK(*cp1a.get("hello").value() == 42);
      CHECK(*cp1a.get("world").value() == 100);
      CHECK(*cp1b.get("hello").value() == 42);
      CHECK(*cp1b.get("world").value() == 20);

      CHECK(cp1a.define("hello", 1).has_value() == false);
      const auto cp2 = cp1a.new_scope();
      const auto cp3 = SILVA_EXPECT_REQUIRE(cp2.define("hello", 4000));
      CHECK(*cp3.get("hello").value() == 4000);
    }
    CHECK(cc.size_total() == 6);
    CHECK(cc.size_occupied() == 1);
    {
      auto cp = cc.root();
      cp      = SILVA_EXPECT_REQUIRE(cp.define("hello", 42));
      cp      = SILVA_EXPECT_REQUIRE(cp.define("world", 8));
      CHECK(*cp.get("hello").value() == 42);
    }
    CHECK(cc.size_occupied() == 1);
  }
}
