#include "fragmentization.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("fragmentization-data", "[fragmentization_t]")
  {
    const unicode::table_t<fragment_category_t>& ft = silva::fragment_table;
    SILVA_EXPECT_REQUIRE(ft.validate());
    REQUIRE(ft.key_size() == 0x110000);

    using enum fragment_category_t;
    CHECK(ft[U'\n'] == Newline);
    CHECK(ft[U' '] == Space);
    CHECK(ft[U'*'] == Operator);
    CHECK(ft[U'⊙'] == Operator);
    CHECK(ft[U'«'] == ParenthesisLeft);
    CHECK(ft[U'»'] == ParenthesisRight);
    CHECK(ft[U'8'] == XID_Continue);
    CHECK(ft[U'A'] == XID_Start);
    CHECK(ft[U'_'] == XID_Start);
  }

  TEST_CASE("fragmentization", "[fragmentization_t]") {}
}
