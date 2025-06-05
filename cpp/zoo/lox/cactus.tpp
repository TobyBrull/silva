#include "cactus.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("cactus", "[cactus_t]")
  {
    cactus_t<string_t, int> cc;
    auto cr = cc.root();
    {
      auto scope      = cr.make_child_arm();
      const int* res1 = SILVA_EXPECT_REQUIRE(scope.define("hello", 42));
      const auto res2 = SILVA_EXPECT_REQUIRE(scope.define("world", 100));
      CHECK(*res1 == 42);
      CHECK(*res2 == 100);
      CHECK(*scope.get("hello") == 42);
      CHECK(*scope.get("world") == 100);

      CHECK(scope.define("hello", 1).has_value() == false);
      const auto sub_scope = scope.make_child_arm();
      const auto res4      = SILVA_EXPECT_REQUIRE(sub_scope.define("hello", 4000));
      CHECK(*res4 == 4000);
      CHECK(*sub_scope.get("world") == 100);
      CHECK(*sub_scope.get("hello") == 4000);
    }
    CHECK(cc.size_total() == 3);
    CHECK(cc.size_occupied() == 1);
  }
}
