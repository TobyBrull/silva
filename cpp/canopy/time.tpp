#include "time.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("time")
  {
    using namespace std::chrono_literals;
    time_point_t tp;
    CHECK(tp.is_none());
    CHECK(tp.to_string_default() == "time_point_none");
    CHECK(tp.to_string_ostream() == "time_point_none");
    time_span_t ts{};
    CHECK(ts.to_string_default() == "0/00:00:00/000.000.000");

    tp = time_point_t{1746363824123456789};
    CHECK(!tp.is_none());
    CHECK(tp.to_string_default() == "2025-05-04/13:03:44/123.456.789");
    CHECK(tp.to_string_ostream() == "2025-05-04 13:03:44.123456789");

    tp = SILVA_EXPECT_REQUIRE(time_point_t::from_string_default("2025-05-04/13:32:29/123.456.789"));
    CHECK(!tp.is_none());
    CHECK(tp.to_string_default() == "2025-05-04/13:32:29/123.456.789");
    CHECK(tp.to_string_ostream() == "2025-05-04 13:32:29.123456789");

    tp = SILVA_EXPECT_REQUIRE(time_point_t::from_string_ostream("2025-05-04 13:32:29.123456789"));
    CHECK(!tp.is_none());
    CHECK(tp.to_string_default() == "2025-05-04/13:32:29/123.456.789");
    CHECK(tp.to_string_ostream() == "2025-05-04 13:32:29.123456789");

    const auto tp1  = "2025-05-04/13:32:29/123.456.789"_time_point;
    const auto tp2  = "2026-06-05/14:33:30/124.457.790"_time_point;
    const auto ts12 = "397/01:01:01/001.001.001"_time_span;
    CHECK(tp2 - tp1 == ts12);
    CHECK(tp1 + ts12 == tp2);
  }
}
