#include "expected.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  expected_t<int> func_1(const int x)
  {
    SILVA_EXPECT(x > 0, MINOR, "x={} must be positive", x);
    return x * 10;
  }

  expected_t<int> func_2(const int x)
  {
    const int y = SILVA_EXPECT_FWD(func_1(x), "failed calling func_1({})", x);
    return y + 2;
  }

  TEST_CASE("expected", "[expected_t]")
  {
    {
      const int result = SILVA_EXPECT_REQUIRE(func_2(1));
      CHECK(result == 12);
    }

    {
      const expected_t<int> result = func_2(-4);
      REQUIRE(!result.has_value());
      constexpr string_view_t expected = R"(
  x=-4 must be positive
failed calling func_1(-4)
)";
      CHECK(to_string(result.error()) == expected.substr(1));
    }
  }
}
