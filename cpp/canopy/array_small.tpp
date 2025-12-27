#include "array_small.hpp"

#include <catch2/catch_all.hpp>

namespace silva::test {
  TEST_CASE("short_vector", "[short_vector_t]")
  {
    array_small_t<int, 4> sv;
    CHECK(sv.size == 0);
    sv.emplace_back(42);
    CHECK(sv.size == 1);
    sv.emplace_back(8);
    CHECK(sv.size == 2);
    sv.emplace_back(4);
    CHECK(sv.size == 3);
    sv.emplace_back(2);
    CHECK(sv.size == 4);
    CHECK(sv[0] == 42);
    CHECK(sv[1] == 8);
    CHECK(sv[2] == 4);
    CHECK(sv[3] == 2);

    array_small_t<int, 4> sv2 = sv;
    CHECK(sv2.size == 4);
    CHECK(sv2[0] == 42);
    CHECK(sv2[1] == 8);
    CHECK(sv2[2] == 4);
    CHECK(sv2[3] == 2);

    sv2[1] = 80;

    sv = sv2;
    CHECK(sv[1] == 80);
  }
}
